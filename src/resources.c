#include "resources.h"
#include "memory.h"

struct ibv_cq *rdma_send_cq, *rdma_recv_cq;
struct ibv_qp *rdma_qp;
pthread_mutex_t rdma_connect_mutex;

void alloc_queue_state(
		struct ibv_comp_channel **cchannel,
		struct ibv_cq **send_cq,
		struct ibv_cq **recv_cq,
		struct ibv_qp **qp,
		uint16_t service_id
		) {
	uint32_t i;

	if (cchannel) { //completion channel is optional
		*cchannel = ibv_create_comp_channel(context.device_context);
		if (*cchannel == NULL) {
			ERROR("Failed to allocate completion event channel!");
			goto error;
			return;
		} else {
			DEBUG("Allocated completion event channel");
		}
	}

	*recv_cq = ibv_create_cq(
			context.device_context,
			100,
			NULL,
			cchannel ? *cchannel : NULL,
			0);
	if (*recv_cq == NULL) {
		ERROR("Failed to allocate receive completion queue!");
		goto error;
		return;
	} else {
		DEBUG("Allocated receive completion queue: %u", (*recv_cq)->handle);
	}

	*send_cq = ibv_create_cq(
			context.device_context,
			100,
			NULL,
			NULL,
			0);
	if (*send_cq == NULL) {
		ERROR("Failed to allocate send completion queue!");
		goto error;
		return;
	} else {
		DEBUG("Allocated send completion queue: %u", (*send_cq)->handle);
	}

	ibv_req_notify_cq(*recv_cq, 0);

	struct ibv_qp_init_attr init_attr = {
		.send_cq = *send_cq,
		.recv_cq = *recv_cq,
		.cap     = {
			.max_send_wr  = NUM_RECV_BUFFERS * 200,
			.max_recv_wr  = NUM_RECV_BUFFERS * 200,
			.max_send_sge = 10,
			.max_recv_sge = 10
		},
		.qp_type = IBV_QPT_UD,
		.sq_sig_all = 0
	};
	*qp = ibv_create_qp(
			context.pd,
			&init_attr);
	if (qp == NULL) {
		ERROR("Failed to allocate queue pair!");
		goto error;
		return;
	} else {
		DEBUG("Allocated queue pair: %u", (*qp)->qp_num);
	}

	struct ibv_qp_attr attr;
	attr.qp_state = IBV_QPS_INIT;
	attr.pkey_index = 0;
	attr.port_num = 1;
	attr.qkey = service_id;

	if (ibv_modify_qp(*qp,
			&attr,
			IBV_QP_STATE		|
			IBV_QP_PKEY_INDEX	|
			IBV_QP_PORT			|
			IBV_QP_QKEY			)) {
		ERROR("Failed to modify QP state to INIT");
		goto error;
		return;
	} else {
		DEBUG("Modified state of QP %u to INIT",(*qp)->qp_num);
	}

	attr.qp_state = IBV_QPS_RTR;
	if (ibv_modify_qp(*qp, &attr, IBV_QP_STATE)) {
		ERROR("Failed to modify QP state to RTR");
		goto error;
		return;
	} else {
		DEBUG("Modified state of QP %u to RTR",(*qp)->qp_num);
	}

	attr.qp_state = IBV_QPS_RTS;
	attr.sq_psn = 0;
	if (ibv_modify_qp(*qp, &attr, IBV_QP_STATE | IBV_QP_SQ_PSN)) {
		ERROR("Failed to modify QP state to RTS");
		goto error;
		return;
	} else {
		DEBUG("Modified state of QP %u to RTS",(*qp)->qp_num);
	}

#ifdef HAVE_DEBUG
	ibv_query_qp(*qp,&attr,~0,&init_attr);
	DEBUG("qkey of QP %u is %#x", (*qp)->qp_num, attr.qkey);
#endif

	for (i = 0; i < NUM_RECV_BUFFERS; ++i) {
		post_new_recv_buf(*qp);
	}

	return;

	error:
	if (*qp) {
		ibv_destroy_qp(*qp);
		*qp = NULL;
	}
	if (*recv_cq) {
		ibv_destroy_cq(*recv_cq);
		*recv_cq = NULL;
	}
	if (*send_cq) {
		ibv_destroy_cq(*send_cq);
		*send_cq = NULL;
	}
	if (*cchannel) {
		ibv_destroy_comp_channel(*cchannel);
		*cchannel = NULL;
	}
}

void create_rdma_connection(uint16_t src, uint16_t dest) {
       DEBUG("Allocating connection state between src %u and dest %u", src, dest);

       //no lock, this shouldn't be touched after creation
       struct service_id *local = context.services[src];
       assert(local); //if we don't have this, something definitely went wrong

       /*
        * we might not have a remote context yet if this is the first send
        * to this destination.
        * No lock as resolve() is supposedly thread-safe and multiple, concurrent
        * resolve()-calls shouldn't hurt (at least not more than holding the
        * lock until resolve() returns.
        */
       if (! context.remotes[dest])
               resolve(src, dest);
       assert(context.remotes[dest]);
       struct remote_context *remote = context.remotes[dest];

       pthread_mutex_lock(&remotes_mutex);
       if (remote->state == RIPC_RDMA_ESTABLISHED) {
               //another thread beat us here
               pthread_mutex_unlock(&remotes_mutex);
               return;
       }
       if (remote->state == RIPC_RDMA_CONNECTING) {
               //another thread is in the process of connecting, but not finished yet
               pthread_mutex_unlock(&remotes_mutex);
               //fixme: sleep() is nasty, find something better
               sleep(1); //give the other guy some time to finish
               return;
       }

       /*
        * Now setup connection state.
        * Note that there is no completion channel here, as we only ever do
        * rdma on this qp. We do need the cqs though to wait for completion of
        * certain events.
        * TODO: Waiting spins on the cqs at the moment. Is that wise?
        */
       remote->state = RIPC_RDMA_CONNECTING;

       remote->rdma_cchannel = ibv_create_comp_channel(context.device_context);
       if (remote->rdma_cchannel == NULL) {
               ERROR("Failed to allocate rdma completion channel!");
               goto error;
               return;
       } else {
               DEBUG("Allocated rdma completion channel: %u", remote->rdma_cchannel->fd);
       }

       remote->rdma_recv_cq = ibv_create_cq(
                       context.device_context,
                       100,
                       NULL,
                       NULL,
                       0);
       if (remote->rdma_recv_cq == NULL) {
               ERROR("Failed to allocate receive completion queue!");
               goto error;
               return;
       } else {
               DEBUG("Allocated receive completion queue: %u", remote->rdma_recv_cq->handle);
       }

       remote->rdma_send_cq = ibv_create_cq(
                       context.device_context,
                       100,
                       NULL,
                       remote->rdma_cchannel,
                       0);
       if (remote->rdma_send_cq == NULL) {
               ERROR("Failed to allocate send completion queue!");
               goto error;
               return;
       } else {
               DEBUG("Allocated send completion queue: %u", remote->rdma_send_cq->handle);
       }

       ibv_req_notify_cq(remote->rdma_send_cq, 0);

       //now for the qp. Remember that we need an RC qp here!
       struct ibv_qp_init_attr init_attr = {
               .send_cq = remote->rdma_send_cq,
               .recv_cq = remote->rdma_recv_cq,
               .cap     = {
                       .max_send_wr  = 2000,
                       .max_recv_wr  = 1, //0 doesn't work here as it seems...
                       .max_send_sge = 1, //we don't do scatter-gather for long sends
                       .max_recv_sge = 1,
               },
               .qp_type = IBV_QPT_RC
       };

       remote->rdma_qp = ibv_create_qp(context.pd, &init_attr);
       if (!remote->rdma_qp) {
               ERROR("Failed to allocate rdma QP");
               goto error;
       } else {
               DEBUG("Allocated rdma QP %u", remote->rdma_qp->qp_num);
       }

       struct ibv_qp_attr attr;
       attr.qp_state = IBV_QPS_INIT;
       attr.port_num = 1;
       attr.pkey_index = 0;
       attr.qp_access_flags = IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;

       if (ibv_modify_qp(remote->rdma_qp, &attr,
                       IBV_QP_STATE |
                       IBV_QP_PKEY_INDEX |
                       IBV_QP_PORT |
                       IBV_QP_ACCESS_FLAGS
                       )) {
               ERROR("Failed to modify rdma QP %u state to INIT", remote->rdma_qp->qp_num);
               goto error;
       }

       uint32_t psn = rand() & 0xffffff;
       DEBUG("My psn is %u", psn);

       //This is as far as we can go on our own. Now notify the other side.

       struct ibv_mr *connect_mr = ripc_alloc_recv_buf(sizeof(struct rdma_connect_msg));
       struct rdma_connect_msg *msg = (struct rdma_connect_msg *)connect_mr->addr;
       msg->type = RIPC_RDMA_CONN_REQ;
       msg->qpn = remote->rdma_qp->qp_num;
       msg->psn = psn;
       msg->src_service_id = src;
       msg->dest_service_id = dest;
       msg->lid = context.lid;
       msg->response_qpn = rdma_qp->qp_num;

       struct ibv_sge sge;
       sge.addr = (uint64_t)msg;
       sge.length = sizeof(struct rdma_connect_msg);
       sge.lkey = connect_mr->lkey;

       struct ibv_send_wr wr;
       wr.next = NULL;
       wr.num_sge = 1;
       wr.opcode = IBV_WR_SEND;
       wr.send_flags = IBV_SEND_SIGNALED;
       wr.sg_list = &sge;
       wr.wr_id = 0xdeadbeef;
       wr.wr.ud.ah = remote->ah;
       wr.wr.ud.remote_qpn = remote->resolver_qp;
       wr.wr.ud.remote_qkey = 0xffff;

       struct ibv_send_wr *bad_wr;

       //holding a lock while waiting on the network is BAD(tm)
       pthread_mutex_unlock(&remotes_mutex);

       //we want only one connection request in flight at a time
       pthread_mutex_lock(&rdma_connect_mutex);

       if (ibv_post_send(rdma_qp, &wr, &bad_wr)) {
    	   ERROR("Failed to send rdma connect request");
    	   goto error;
       } else {
    	   DEBUG("Sent rdma connect request to remote %u (qp %u)", dest, remote->resolver_qp);
       }

       struct ibv_wc wc;

       while ( ! ibv_poll_cq(rdma_send_cq, 1, &wc)) { /* wait for send completion */ }

       while ( ! ibv_poll_cq(rdma_recv_cq, 1, &wc)) { /* wait for response */ }

       post_new_recv_buf(rdma_qp);

       pthread_mutex_unlock(&rdma_connect_mutex);

       struct ibv_recv_wr *response_wr =
    		   (struct ibv_recv_wr *)wc.wr_id;
       struct rdma_connect_msg *response_msg =
    		   (struct rdma_connect_msg *)(response_wr->sg_list->addr + 40);

       assert(response_msg->type == RIPC_RDMA_CONN_REPLY);
       assert(response_msg->dest_service_id == dest);

       DEBUG("Received rdma connect reply: from %u, for %u, remote lid: %u, remote psn: %u, remote qpn: %u",
    		   response_msg->src_service_id,
    		   response_msg->dest_service_id,
    		   response_msg->lid,
    		   response_msg->psn,
    		   response_msg->qpn);

       attr.qp_state = IBV_QPS_RTR;
       attr.path_mtu = IBV_MTU_2048;
       attr.dest_qp_num = response_msg->qpn;
       attr.rq_psn = response_msg->psn;
       attr.max_dest_rd_atomic = 1;
       attr.min_rnr_timer = 12;
       attr.ah_attr.is_global = 0;
       attr.ah_attr.dlid = response_msg->lid;
       attr.ah_attr.sl = 0;
       attr.ah_attr.src_path_bits = 0;
       attr.ah_attr.port_num = 1;

       pthread_mutex_lock(&remotes_mutex);

       if (ibv_modify_qp(remote->rdma_qp, &attr,
    		   IBV_QP_STATE              |
    		   IBV_QP_AV                 |
    		   IBV_QP_PATH_MTU           |
    		   IBV_QP_DEST_QPN           |
    		   IBV_QP_RQ_PSN             |
    		   IBV_QP_MAX_DEST_RD_ATOMIC |
    		   IBV_QP_MIN_RNR_TIMER)) {
    	   ERROR("Failed to rdma modify QP %u to RTR", remote->rdma_qp->qp_num);
    	   goto error;
       }

       attr.qp_state 	    = IBV_QPS_RTS;
       attr.timeout 	    = 14;
       attr.retry_cnt 	    = 7;
       attr.rnr_retry 	    = 7;
       attr.sq_psn 	    = psn;
       attr.max_rd_atomic  = 1;
       if (ibv_modify_qp(remote->rdma_qp, &attr,
    		   IBV_QP_STATE              |
    		   IBV_QP_TIMEOUT            |
    		   IBV_QP_RETRY_CNT          |
    		   IBV_QP_RNR_RETRY          |
    		   IBV_QP_SQ_PSN             |
    		   IBV_QP_MAX_QP_RD_ATOMIC)) {
    	   ERROR("Failed to modify rdma QP %u to RTS", remote->rdma_qp->qp_num);
    	   goto error;
       }

       post_new_recv_buf(remote->rdma_qp);

       //all done? Then we're connected now :)
       remote->state = RIPC_RDMA_ESTABLISHED;

#ifdef HAVE_DEBUG
       dump_qp_state(remote->rdma_qp);
#endif

       pthread_mutex_unlock(&remotes_mutex);

       ripc_buf_free(msg);
       ripc_buf_free(response_msg);
       free(response_wr->sg_list);
       free(response_wr);

       return;

       error:
       if (remote->rdma_qp)
               ibv_destroy_qp(remote->rdma_qp);
       if (remote->rdma_recv_cq)
               ibv_destroy_cq(remote->rdma_recv_cq);
       if (remote->rdma_send_cq)
               ibv_destroy_cq(remote->rdma_recv_cq);
       remote->state = RIPC_RDMA_DISCONNECTED;
       pthread_mutex_unlock(&remotes_mutex);
}

void dump_qp_state(struct ibv_qp *qp) {
	struct ibv_qp_attr attr;
	struct ibv_qp_init_attr init_attr;
	ibv_query_qp(qp, &attr, 0xffffffff, &init_attr);

	ERROR("State for QP %u:", qp->qp_num);
	ERROR("QP type: %u", init_attr.qp_type);
	ERROR("dlid: %u", attr.ah_attr.dlid);
	ERROR("dest QP: %u", attr.dest_qp_num);
	ERROR("dest port: %u", attr.ah_attr.port_num);
	ERROR("max inline data: %u", attr.cap.max_inline_data);
	ERROR("max recv sge: %u", attr.cap.max_recv_sge);
	ERROR("max send sge: %u", attr.cap.max_send_sge);
	ERROR("max recv WR: %u", attr.cap.max_recv_wr);
	ERROR("max send WR: %u", attr.cap.max_send_wr);
	ERROR("cur state: %u", attr.cur_qp_state);
	ERROR("state: %u", attr.qp_state);
	ERROR("min RNR timer: %u", attr.min_rnr_timer);
	ERROR("path migration state: %u", attr.path_mig_state);
	ERROR("mtu: %u", attr.path_mtu);
	ERROR("pkey index: %u", attr.pkey_index);
	ERROR("port num: %u", attr.port_num);
	ERROR("qkey: %#lx", attr.qkey);
	ERROR("access flags: %#lx", attr.qp_access_flags);
	ERROR("retry count: %u", attr.retry_cnt);
	ERROR("RNR retry count: %u", attr.rnr_retry);
	ERROR("RQ psn: %u", attr.rq_psn);
	ERROR("SQ psn: %u", attr.sq_psn);
	ERROR("SQ draining: %u", attr.sq_draining);
	ERROR("timeout: %u", attr.timeout);
}












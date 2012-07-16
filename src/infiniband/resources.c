/*  Copyright 2011, 2012 Jens Kehne
 *  Copyright 2012 Jan Stoess, Karlsruhe Institute of Technology
 *
 *  LibRIPC is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License as published by the
 *  Free Software Foundation, either version 2.1 of the License, or (at your
 *  option) any later version.
 *
 *  LibRIPC is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with libRIPC.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <../common.h>
#include <../resources.h>

struct service_id rdma_service_id;
pthread_mutex_t rdma_connect_mutex;
pthread_t async_event_logger_thread;

bool join_and_attach_multicast(struct service_id *service_id) {
	struct mcast_parameters mcg_params;
	struct ibv_port_attr port_attr;
	uint8_t sid_high = service_id->number >> 8;
	uint8_t sid_low = service_id->number & 0xff;

	//mcg_params.user_mgid = NULL; //NULL means "default"
						//			0xde  ad  be  ef  ca  fe
	mcg_params.user_mgid = (char *)malloc(50);
	sprintf(mcg_params.user_mgid, "255:1:0:0:222:173:190:239:%u:%u:0:0:0:0:0:1", sid_high, sid_low);
	set_multicast_gid(&mcg_params, service_id->na.qp->qp_num, 1);

	if (ibv_query_gid(context.na.device_context, 1, 0, &mcg_params.port_gid)) {
			return false;
	}

	if (ibv_query_pkey(context.na.device_context, 1, DEF_PKEY_IDX, &mcg_params.pkey)) {
		return false;
	}

	if (ibv_query_port(context.na.device_context, context.na.port_num, &port_attr)) {
		return false;
	}
	mcg_params.ib_devname = NULL;
	mcg_params.sm_lid  = port_attr.sm_lid;
	mcg_params.sm_sl   = port_attr.sm_sl;
	mcg_params.ib_port = 1;

	DEBUG("Prepared multicast descriptor item for service ID %u", service_id->number);

	/*
	 * To do multicast, we first need to tell the fabric to create a multicast
	 * group, and then attach ourselves to it. join_multicast_group uses libumad
	 * to send the necessary management packets to create a multicast group
	 * with a random LID.
	 */
	if (join_multicast_group(SUBN_ADM_METHOD_SET,&mcg_params)) {
		ERROR("Failed to join multicast group for service ID %u", service_id->number);
		return false;
	}

	DEBUG("Successfully created multicast group with LID %#x and GID %lx:%lx for service ID %u",
			mcg_params.mlid,
			mcg_params.mgid.global.subnet_prefix,
			mcg_params.mgid.global.interface_id,
			service_id->number);

	/*
	 * Now that our multicast group exists, we need to attach a QP (or more)
	 * to it. Every QP attached to the group will receive all packets sent to
	 * its LID.
	 */
	int ret;
	if (ret =/*=*/ ibv_attach_mcast(service_id->na.qp,&mcg_params.mgid,mcg_params.mlid)) {
		ERROR("Couldn't attach QP to multicast group for service ID %u (%s)", service_id->number, strerror(ret));
		return false;
	}

	DEBUG("Successfully attached to multicast group for service ID %u", service_id->number);

	//cache address handle used for sending requests
	struct ibv_ah_attr ah_attr;
	ah_attr.dlid = mcg_params.mlid;
	ah_attr.is_global = 1;
	ah_attr.sl = 0;
	ah_attr.port_num = context.na.port_num;
	ah_attr.grh.dgid = mcg_params.mgid;
	ah_attr.grh.sgid_index = 0;
	ah_attr.grh.hop_limit = 1;
	ah_attr.src_path_bits = 0;

	service_id->na.mcast_ah = ibv_create_ah(context.na.pd, &ah_attr);

	if (!service_id->na.mcast_ah) {
		ERROR("Failed to create resolver address handle");
		return false;
	}

	DEBUG("Successfully created resolver address handle");

	return true;
}

void alloc_queue_state(struct service_id *service_id) {
	uint32_t i;

	if (!service_id->na.no_cchannel) { //completion channel is optional
		service_id->na.cchannel = 
                        ibv_create_comp_channel(context.na.device_context);
		if (service_id->na.cchannel == NULL) {
			ERROR("Failed to allocate completion event channel!");
			goto error;
			return;
		} else {
			DEBUG("Allocated completion event channel");
		}
	} else {
		DEBUG("Skipping allocation of completion event channel for service ID %u", service_id->number);
	}

	service_id->na.recv_cq = ibv_create_cq(
                context.na.device_context,
                100,
                NULL,
                service_id->na.no_cchannel ? NULL : service_id->na.cchannel,
                0);
	if (service_id->na.recv_cq == NULL) {
		ERROR("Failed to allocate receive completion queue!");
		goto error;
		return;
	} else {
		DEBUG("Allocated receive completion queue: %u", (service_id->na.recv_cq)->handle);
	}

	service_id->na.send_cq = ibv_create_cq(
                context.na.device_context,
                100,
                NULL,
                NULL,
                0);
	if (service_id->na.send_cq == NULL) {
		ERROR("Failed to allocate send completion queue!");
		goto error;
		return;
	} else {
		DEBUG("Allocated send completion queue: %u", (service_id->na.send_cq)->handle);
	}

	ibv_req_notify_cq(service_id->na.recv_cq, 0);

	struct ibv_qp_init_attr init_attr = {
		.send_cq = service_id->na.send_cq,
		.recv_cq = service_id->na.recv_cq,
		.cap     = {
			.max_send_wr  = NUM_RECV_BUFFERS * 200,
			.max_recv_wr  = NUM_RECV_BUFFERS * 200,
			.max_send_sge = 10,
			.max_recv_sge = 10
		},
		.qp_type = IBV_QPT_UD,
		.sq_sig_all = 0
	};
	service_id->na.qp = ibv_create_qp(
                context.na.pd,
                &init_attr);
	if (service_id->na.qp == NULL) {
		ERROR("Failed to allocate queue pair!");
		goto error;
		return;
	} else {
		DEBUG("Allocated queue pair: %u", (service_id->na.qp)->qp_num);
	}

	struct ibv_qp_attr attr;
	attr.qp_state = IBV_QPS_INIT;
	attr.pkey_index = 0;
	attr.port_num = context.na.port_num;
	attr.qkey = service_id->is_multicast ? 0xffff : service_id->number;

	if (ibv_modify_qp(service_id->na.qp,
                          &attr,
                          IBV_QP_STATE		|
                          IBV_QP_PKEY_INDEX	|
                          IBV_QP_PORT			|
                          IBV_QP_QKEY			)) {
		ERROR("Failed to modify QP state to INIT");
		goto error;
		return;
	} else {
		DEBUG("Modified state of QP %u to INIT",(service_id->na.qp)->qp_num);
	}

	attr.qp_state = IBV_QPS_RTR;
	if (ibv_modify_qp(service_id->na.qp, &attr, IBV_QP_STATE)) {
		ERROR("Failed to modify QP state to RTR");
		goto error;
		return;
	} else {
		DEBUG("Modified state of QP %u to RTR",(service_id->na.qp)->qp_num);
	}

	attr.qp_state = IBV_QPS_RTS;
	attr.sq_psn = 0;
	if (ibv_modify_qp(service_id->na.qp, &attr, IBV_QP_STATE | IBV_QP_SQ_PSN)) {
		ERROR("Failed to modify QP state to RTS");
		goto error;
		return;
	} else {
		DEBUG("Modified state of QP %u to RTS",(service_id->na.qp)->qp_num);
	}

#ifdef HAVE_DEBUG
	ibv_query_qp(service_id->na.qp,&attr,~0,&init_attr);
	DEBUG("qkey of QP %u is %#x", (service_id->na.qp)->qp_num, attr.qkey);
#endif

	if (service_id->is_multicast)
		if (!join_and_attach_multicast(service_id))
			goto error;

	for (i = 0; i < NUM_RECV_BUFFERS; ++i) {
		post_new_recv_buf(service_id->na.qp);
	}

	return;

error:
	if (service_id->na.qp) {
		ibv_destroy_qp(service_id->na.qp);
		service_id->na.qp = NULL;
	}
	if (service_id->na.recv_cq) {
		ibv_destroy_cq(service_id->na.recv_cq);
		service_id->na.recv_cq = NULL;
	}
	if (service_id->na.send_cq) {
		ibv_destroy_cq(service_id->na.send_cq);
		service_id->na.send_cq = NULL;
	}
	if (service_id->na.cchannel) {
		ibv_destroy_comp_channel(service_id->na.cchannel);
		service_id->na.cchannel = NULL;
	}
	if (service_id->na.mcast_ah) {
		ibv_destroy_ah(service_id->na.mcast_ah);
		service_id->na.mcast_ah = NULL;
	}
}

void create_rdma_connection(uint16_t src, uint16_t dest) {
       DEBUG("Allocating connection state between src %u and dest %u", src, dest);

       //no lock, this shouldn't be touched after creation
       struct service_id *local = context.services[src];
       int ret;
       assert(local); //if we don't have this, something definitely went wrong

       /*
        * we might not have a remote context yet if this is the first send
        * to this destination.
        * No lock as resolve() is supposedly thread-safe and multiple, concurrent
        * resolve()-calls shouldn't hurt (at least not more than holding the
        * lock until resolve() returns.
        */
       if (( ! context.remotes[dest])
    		   || ( ! context.remotes[dest]->na.ah)
    		   || ( ! context.remotes[dest]->resolver_qp))
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

       remote->na.rdma_cchannel = ibv_create_comp_channel(context.na.device_context);
       if (remote->na.rdma_cchannel == NULL) {
               ERROR("Failed to allocate rdma completion channel!");
               goto error;
               return;
       } else {
               DEBUG("Allocated rdma completion channel: %u", remote->na.rdma_cchannel->fd);
       }

       remote->na.rdma_recv_cq = ibv_create_cq(
                       context.na.device_context,
                       100,
                       NULL,
                       NULL,
                       0);
       if (remote->na.rdma_recv_cq == NULL) {
               ERROR("Failed to allocate receive completion queue!");
               goto error;
               return;
       } else {
               DEBUG("Allocated receive completion queue: %u", remote->na.rdma_recv_cq->handle);
       }

       remote->na.rdma_send_cq = ibv_create_cq(
                       context.na.device_context,
                       100,
                       NULL,
                       remote->na.rdma_cchannel,
                       0);
       if (remote->na.rdma_send_cq == NULL) {
               ERROR("Failed to allocate send completion queue!");
               goto error;
               return;
       } else {
               DEBUG("Allocated send completion queue: %u", remote->na.rdma_send_cq->handle);
       }

       ibv_req_notify_cq(remote->na.rdma_send_cq, 0);

       //now for the qp. Remember that we need an RC qp here!
       struct ibv_qp_init_attr init_attr = {
               .send_cq = remote->na.rdma_send_cq,
               .recv_cq = remote->na.rdma_recv_cq,
               .cap     = {
                       .max_send_wr  = 2000,
                       .max_recv_wr  = 1, //0 doesn't work here as it seems...
                       .max_send_sge = 1, //we don't do scatter-gather for long sends
                       .max_recv_sge = 1,
               },
               .qp_type = IBV_QPT_RC
       };

       remote->na.rdma_qp = ibv_create_qp(context.na.pd, &init_attr);
       if (!remote->na.rdma_qp) {
               ERROR("Failed to allocate rdma QP");
               goto error;
       } else {
               DEBUG("Allocated rdma QP %u", remote->na.rdma_qp->qp_num);
       }

       struct ibv_qp_attr attr;
       attr.qp_state = IBV_QPS_INIT;
       attr.port_num = context.na.port_num;
       attr.pkey_index = 0;
       attr.qp_access_flags = IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;

       if (ibv_modify_qp(remote->na.rdma_qp, &attr,
                       IBV_QP_STATE |
                       IBV_QP_PKEY_INDEX |
                       IBV_QP_PORT |
                       IBV_QP_ACCESS_FLAGS
                       )) {
               ERROR("Failed to modify rdma QP %u state to INIT", remote->na.rdma_qp->qp_num);
               goto error;
       }

       uint32_t psn = rand() & 0xffffff;
       DEBUG("My psn is %u", psn);

       //This is as far as we can go on our own. Now notify the other side.

       struct ibv_mr *connect_mr = ripc_alloc_recv_buf(sizeof(struct rdma_connect_msg)).na;
       struct rdma_connect_msg *msg = (struct rdma_connect_msg *)connect_mr->addr;
       msg->type = RIPC_RDMA_CONN_REQ;
       msg->qpn = remote->na.rdma_qp->qp_num;
       msg->psn = psn;
       msg->src_service_id = src;
       msg->dest_service_id = dest;
       msg->lid = context.na.lid;
       msg->response_qpn = rdma_service_id.na.qp->qp_num;

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
       wr.wr.ud.ah = remote->na.ah;
       wr.wr.ud.remote_qpn = remote->resolver_qp;
       wr.wr.ud.remote_qkey = 0xffff;

       struct ibv_send_wr *bad_wr = NULL;

       //holding a lock while waiting on the network is BAD(tm)
       pthread_mutex_unlock(&remotes_mutex);

       //we want only one connection request in flight at a time
       pthread_mutex_lock(&rdma_connect_mutex);
retry:
	   ret = ibv_post_send(rdma_service_id.na.qp, &wr, &bad_wr);
       if (ret) {
    	   ERROR("Failed to send rdma connect request: %s", strerror(ret));
    	   goto error;
       } else {
    	   DEBUG("Sent rdma connect request to remote %u (qp %u)", dest, remote->resolver_qp);
       }

       DEBUG("RDMA QP state: %u", rdma_service_id.na.qp->state);

       struct ibv_wc wc;

       while ( ! ibv_poll_cq(rdma_service_id.na.send_cq, 1, &wc)) { /* wait for send completion */ }

       assert(wc.status == IBV_WC_SUCCESS);

       DEBUG("Got send completion for connect request");

       int i = 0;
       while ( ! ibv_poll_cq(rdma_service_id.na.recv_cq, 1, &wc)) {
    	   if (i++ > 100000000) {
    		   DEBUG("Timeout during rdma connect request (remote %u)", dest);
    		   goto retry;
    	   }
       }

       assert(wc.status == IBV_WC_SUCCESS);

       post_new_recv_buf(rdma_service_id.na.qp);

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
       attr.ah_attr.port_num = context.na.port_num;

       pthread_mutex_lock(&remotes_mutex);

       if (ibv_modify_qp(remote->na.rdma_qp, &attr,
    		   IBV_QP_STATE              |
    		   IBV_QP_AV                 |
    		   IBV_QP_PATH_MTU           |
    		   IBV_QP_DEST_QPN           |
    		   IBV_QP_RQ_PSN             |
    		   IBV_QP_MAX_DEST_RD_ATOMIC |
    		   IBV_QP_MIN_RNR_TIMER)) {
    	   ERROR("Failed to rdma modify QP %u to RTR", remote->na.rdma_qp->qp_num);
    	   goto error;
       }

       attr.qp_state 	    = IBV_QPS_RTS;
       attr.timeout 	    = 14;
       attr.retry_cnt 	    = 7;
       attr.rnr_retry 	    = 7;
       attr.sq_psn 	    = psn;
       attr.max_rd_atomic  = 1;
       if (ibv_modify_qp(remote->na.rdma_qp, &attr,
    		   IBV_QP_STATE              |
    		   IBV_QP_TIMEOUT            |
    		   IBV_QP_RETRY_CNT          |
    		   IBV_QP_RNR_RETRY          |
    		   IBV_QP_SQ_PSN             |
    		   IBV_QP_MAX_QP_RD_ATOMIC)) {
    	   ERROR("Failed to modify rdma QP %u to RTS", remote->na.rdma_qp->qp_num);
    	   goto error;
       }

       post_new_recv_buf(remote->na.rdma_qp);

       //all done? Then we're connected now :)
       remote->state = RIPC_RDMA_ESTABLISHED;

#ifdef HAVE_DEBUG
       dump_qp_state(remote->na.rdma_qp);
#endif

       pthread_mutex_unlock(&remotes_mutex);

       ripc_buf_free(msg);
       ripc_buf_free(response_msg);
       free(response_wr->sg_list);
       free(response_wr);

       return;

       error:
       if (remote->na.rdma_qp)
               ibv_destroy_qp(remote->na.rdma_qp);
       if (remote->na.rdma_recv_cq)
               ibv_destroy_cq(remote->na.rdma_recv_cq);
       if (remote->na.rdma_send_cq)
               ibv_destroy_cq(remote->na.rdma_recv_cq);
       remote->state = RIPC_RDMA_DISCONNECTED;
       pthread_mutex_unlock(&remotes_mutex);
}

void* async_event_logger(void *context) {
	struct ibv_async_event async_event;
	DEBUG("Asynchronous event logger started");
	while (true) {
		if (ibv_get_async_event((struct ibv_context *)context, &async_event))
			continue;
		DEBUG("Received asynchronous event: %s", ibv_event_type_str(async_event.event_type));
		ibv_ack_async_event(&async_event);
	}
	return NULL;
}

void dump_qp_state(struct ibv_qp *qp) {
	struct ibv_qp_attr attr;
	struct ibv_qp_init_attr init_attr;
	ibv_query_qp(qp, &attr, 0xffffffff, &init_attr);

	ERROR("State for QP %u:", qp->qp_num);
	ERROR("QP type: %u", init_attr.qp_type);
	ERROR("dlid: %u", attr.ah_attr.dlid);
	ERROR("dest QP: %u", attr.dest_qp_num);
	ERROR("local port number: %u", attr.ah_attr.port_num);
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
	ERROR("QP local port num: %u", attr.port_num);
	ERROR("qkey: %#x", attr.qkey);
	ERROR("access flags: %#x", attr.qp_access_flags);
	ERROR("retry count: %u", attr.retry_cnt);
	ERROR("RNR retry count: %u", attr.rnr_retry);
	ERROR("RQ psn: %u", attr.rq_psn);
	ERROR("SQ psn: %u", attr.sq_psn);
	ERROR("SQ draining: %u", attr.sq_draining);
	ERROR("timeout: %u", attr.timeout);
}












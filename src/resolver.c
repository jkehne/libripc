#include "resolver.h"
#include "ripc.h"
#include "multicast_resources.h"
#include "resources.h"
#include "memory.h"
#include <string.h>
#include <pthread.h>

struct ibv_cq *mcast_send_cq, *mcast_recv_cq, *unicast_send_cq, *unicast_recv_cq;
struct ibv_qp *mcast_qp, *unicast_qp;
struct ibv_comp_channel *mcast_cchannel;
struct ibv_ah *mcast_ah;
pthread_t responder_thread;

/*
 * TODO:
 * - Make this a separate thread
 * - Check if the multicast group already exists, and if so, attach to it
 * - Of course, wait for requests and create responses :)
 */

void handle_rdma_connect(struct rdma_connect_msg *msg) {
	DEBUG("Received rdma connect message: from %u, for %u, remote lid: %u, remote psn: %u, remote qpn: %u",
			msg->src_service_id,
			msg->dest_service_id,
			msg->lid,
			msg->psn,
			msg->qpn);

    if (! context.remotes[msg->src_service_id])
            resolve(msg->dest_service_id, msg->src_service_id);
    assert(context.remotes[msg->src_service_id]);
    struct remote_context *remote = context.remotes[msg->src_service_id];

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
                    NULL,
                    0);
    if (remote->rdma_send_cq == NULL) {
            ERROR("Failed to allocate send completion queue!");
            goto error;
            return;
    } else {
            DEBUG("Allocated send completion queue: %u", remote->rdma_send_cq->handle);
    }

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

    /*
     * now we have the same state as the other side, but we have the information
     * the other side is missing. We complete the process, then we send the reply.
     * We could send the reply first, which would make the remaining establishement
     * process concurrent, but that might raise synchronization issues.
     */

	attr.qp_state = IBV_QPS_RTR;
	attr.path_mtu = IBV_MTU_2048;
	attr.dest_qp_num = msg->qpn;
	attr.rq_psn = msg->psn;
	attr.max_dest_rd_atomic = 1;
	attr.min_rnr_timer = 12;
	attr.ah_attr.is_global = 0;
	attr.ah_attr.dlid = msg->lid;
	attr.ah_attr.sl = 0;
	attr.ah_attr.src_path_bits = 0;
	attr.ah_attr.port_num = 1;

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
	attr.sq_psn 	    = 0;
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

	//all done? Then we're connected now :)
	remote->state = RIPC_RDMA_ESTABLISHED;

	//now send a reply to let the other side know our details
	msg->lid = context.lid;
	msg->qpn = remote->rdma_qp->qp_num;
	msg->psn = 0;
	//msg->response_qpn = rdma_qp->qp_num;
	msg->type = RIPC_RDMA_CONN_REPLY;

	struct ibv_mr *msg_mr = used_buf_list_get(msg);
	assert(msg_mr);

	struct ibv_sge sge;
	sge.addr = (uint64_t)msg;
	sge.length = sizeof(struct rdma_connect_msg);
	sge.lkey = msg_mr->lkey;

	struct ibv_send_wr wr;
	wr.next = NULL;
	wr.num_sge = 1;
	wr.opcode = IBV_WR_SEND;
	wr.send_flags = IBV_SEND_SIGNALED;
	wr.sg_list = &sge;
	wr.wr_id = 0xdeadbeef;
	wr.wr.ud.ah = remote->ah;
	wr.wr.ud.remote_qkey = 0xffff;
	wr.wr.ud.remote_qpn = msg->response_qpn;

	struct ibv_send_wr *bad_wr;

    //holding a lock while waiting on the network is BAD(tm)
    pthread_mutex_unlock(&remotes_mutex);

	if (ibv_post_send(unicast_qp, &wr, &bad_wr)) {
		ERROR("Failed to send connect response to remote %u (qp %u)",
				msg->src_service_id,
				msg->response_qpn);
		goto error;
	} else {
		DEBUG("Sent rdma connect response to remote %u (qp %u)",
				msg->src_service_id,
				msg->response_qpn);
	}

	struct ibv_wc wc;

	while ( ! ibv_poll_cq(unicast_qp, 1, &wc)) { /* wait */ }

	//msg is freed in caller!
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

void *start_responder(void *arg) {
	DEBUG("Allocating responder state");

	uint32_t i;

	//multicast state, to wait for requests
	alloc_queue_state(
			&mcast_cchannel,
			&mcast_send_cq,
			&mcast_recv_cq,
			&mcast_qp,
			0xffff
			);

	//unicast state, to send requests
	alloc_queue_state(
			NULL, //no completion channel, as nothing should arrive on this QP
			&unicast_send_cq,
			&unicast_recv_cq,
			&unicast_qp,
			0xffff
			);

	//got all my local state, now register for multicast
	struct mcast_parameters mcg_params;
	struct ibv_port_attr port_attr;

	//mcg_params.user_mgid = NULL; //NULL means "default"
						//			0xde  ad  be  ef  ca  fe
	mcg_params.user_mgid = "255:1:0:0:222:173:190:239:202:254:0:0:0:0:0:1";
	set_multicast_gid(&mcg_params, mcast_qp->qp_num, 1);

	if (ibv_query_gid(context.device_context, 1, 0, &mcg_params.port_gid)) {
			return NULL;
	}

	if (ibv_query_pkey(context.device_context, 1, DEF_PKEY_IDX, &mcg_params.pkey)) {
		return NULL;
	}

	if (ibv_query_port(context.device_context, 1, &port_attr)) {
		return NULL;
	}
	mcg_params.ib_devname = NULL;
	mcg_params.sm_lid  = port_attr.sm_lid;
	mcg_params.sm_sl   = port_attr.sm_sl;
	mcg_params.ib_port = 1;

	DEBUG("Prepared multicast descriptor item");

	/*
	 * To do multicast, we first need to tell the fabric to create a multicast
	 * group, and then attach ourselves to it. join_multicast_group uses libumad
	 * to send the necessary management packets to create a multicast group
	 * with a random LID.
	 */
	if (join_multicast_group(SUBN_ADM_METHOD_SET,&mcg_params)) {
		ERROR(" Failed to Join Mcast request\n");
		return NULL;
	}

	DEBUG("Successfully created multicast group with LID %#x and GID %lx:%lx",
			mcg_params.mlid,
			mcg_params.mgid.global.subnet_prefix,
			mcg_params.mgid.global.interface_id);

	/*
	 * Now that our multicast group exists, we need to attach a QP (or more)
	 * to it. Every QP attached to the group will receive all packets sent to
	 * its LID.
	 */
	int ret;
	if (ret =/*=*/ ibv_attach_mcast(mcast_qp,&mcg_params.mgid,mcg_params.mlid)) {
		ERROR("Couldn't attach QP to MultiCast group (%s)", strerror(ret));
		return NULL;
	}

	DEBUG("Successfully attached to multicast group");

	//cache address handle used for sending requests
	struct ibv_ah_attr ah_attr;
	ah_attr.dlid = mcg_params.mlid;
	ah_attr.is_global = 1;
	ah_attr.sl = 0;
	ah_attr.port_num = 1;
	ah_attr.grh.dgid = mcg_params.mgid;
	ah_attr.grh.sgid_index = 0;
	ah_attr.grh.hop_limit = 1;
	ah_attr.src_path_bits = 0;

	mcast_ah = ibv_create_ah(context.pd, &ah_attr);

	if (!mcast_ah) {
		ERROR("Failed to create resolver address handle");
		goto error;
	}

	DEBUG("Successfully created resolver address handle");

	//all done, now enter event loop
	struct ibv_cq *recvd_on;
	void *cq_context;
	struct ibv_wc wc;
	struct ibv_recv_wr *wr;
	struct ibv_send_wr resp_wr, *bad_send_wr;
	struct ibv_sge resp_sge;
	struct resolver_msg *msg, *response;
	struct ibv_mr *resp_mr;
	bzero(&ah_attr, sizeof(struct ibv_ah_attr)); //prepare for re-use
	struct ibv_ah *tmp_ah;
	bool for_us;

	//prepare a response as far as possible
	resp_mr = ripc_alloc_recv_buf(sizeof(struct resolver_msg));
	response = (struct resolver_msg *)resp_mr->addr;
	response->lid = context.lid;
	response->type = RIPC_MSG_RESOLVE_REPLY;
	response->resolver_qpn = mcast_qp->qp_num;
	response->response_qpn = unicast_qp->qp_num;

	resp_sge.addr = (uint64_t)response;
	resp_sge.length = sizeof(struct resolver_msg);
	resp_sge.lkey = resp_mr->lkey;

	resp_wr.next = NULL;
	resp_wr.num_sge = 1;
	resp_wr.opcode = IBV_WR_SEND;
	resp_wr.send_flags = IBV_SEND_SIGNALED;
	resp_wr.sg_list = &resp_sge;
	resp_wr.wr_id = 0xdeadbeef;
	resp_wr.wr.ud.remote_qkey = 0xffff;

	while(true) {
		do {
			ibv_get_cq_event(mcast_cchannel, &recvd_on, &cq_context);

			assert(recvd_on == mcast_recv_cq);

			ibv_ack_cq_events(recvd_on, 1);
			ibv_req_notify_cq(recvd_on, 0);
		} while ( ! ibv_poll_cq(recvd_on, 1, &wc));

		post_new_recv_buf(mcast_qp);

		wr = (struct ibv_recv_wr *) wc.wr_id;
		msg = (struct resolver_msg *)(wr->sg_list->addr + 40);

		if (msg->type == RIPC_RDMA_CONN_REQ) {
			handle_rdma_connect((struct rdma_connect_msg *)msg);
			ripc_buf_free(msg);
			free(wr->sg_list);
			free(wr);
			continue;
		}

		//assert(msg->type == RIPC_MSG_RESOLVE_REQ);
		if (msg->type != RIPC_MSG_RESOLVE_REQ) {
			ERROR("Spurious resolver message, discarding");
			ripc_buf_free(msg);
			free(wr->sg_list);
			free(wr);
			continue;
		}

		DEBUG("Received message: from service: %u, for service: %u, from qpn: %u, from lid: %u, response to: %u",
				msg->src_service_id,
				msg->dest_service_id,
				msg->service_qpn,
				msg->lid,
				msg->response_qpn
				);

		/*
		 * First, check if one of our own services has been requested,
		 * and if so, queue a reply for sending. We can update our own
		 * cache while the send takes place.
		 */
		for_us = false;

		pthread_mutex_lock(&services_mutex);

		if (context.services[msg->dest_service_id]) {
			//yay, it's for us!
			DEBUG("Message is for me :)");

			response->service_qpn =
					context.services[msg->dest_service_id]->qp->qp_num;

			pthread_mutex_unlock(&services_mutex);

			for_us = true;
			response->dest_service_id = msg->dest_service_id;
			response->src_service_id = msg->src_service_id;

			ah_attr.dlid = msg->lid;
			ah_attr.port_num = 1;
			resp_wr.wr.ud.ah = ibv_create_ah(context.pd, &ah_attr);
			resp_wr.wr.ud.remote_qpn = msg->response_qpn;

			ibv_post_send(unicast_qp, &resp_wr, &bad_send_wr);
			DEBUG("Sent reply");
		} else
			pthread_mutex_unlock(&services_mutex);

		/*
		 * Now, cache the requestor's contact info, just in case we
		 * want to send him a message in the future.
		 */
		ah_attr.dlid = msg->lid;
		ah_attr.port_num = 1;
		tmp_ah = ibv_create_ah(context.pd, &ah_attr);

		pthread_mutex_lock(&remotes_mutex);

		if (!context.remotes[msg->src_service_id]) {
			context.remotes[msg->src_service_id] =
					malloc(sizeof(struct remote_context));
			bzero(context.remotes[msg->src_service_id], sizeof(struct remote_context));
			context.remotes[msg->src_service_id]->state = RIPC_RDMA_DISCONNECTED;
		}

		context.remotes[msg->src_service_id]->ah = tmp_ah;
		context.remotes[msg->src_service_id]->qp_num = msg->service_qpn;
		context.remotes[msg->src_service_id]->resolver_qp = msg->resolver_qpn;

		pthread_mutex_unlock(&remotes_mutex);
		DEBUG("Cached remote contact info");

		/*
		 * Finally, poll for the send completion if necessary
		 */
		if (for_us) {
			while (!ibv_poll_cq(unicast_send_cq, 1, &wc)) { /* wait */ }
			DEBUG("Got send completion, result: %u", wc.status);

			//we won't be needing the response ah for a while
			ibv_destroy_ah(resp_wr.wr.ud.ah);
		}

		ripc_buf_free(msg);
		free(wr->sg_list);
		free(wr);
	}

	return NULL;

	error:
	if (mcast_qp)
		ibv_destroy_qp(mcast_qp);
	if (mcast_recv_cq)
		ibv_destroy_cq(mcast_recv_cq);
	if (mcast_send_cq)
		ibv_destroy_cq(mcast_send_cq);
	if (mcast_cchannel)
		ibv_destroy_comp_channel(mcast_cchannel);
	if (unicast_qp)
		ibv_destroy_qp(unicast_qp);
	if (unicast_recv_cq)
		ibv_destroy_cq(unicast_recv_cq);
	if (unicast_send_cq)
		ibv_destroy_cq(unicast_send_cq);
	if (mcast_ah)
		ibv_destroy_ah(mcast_ah);

	return NULL;
}

void dispatch_responder(void) {
	//just trampoline to the real init function in new thread
	pthread_create(&responder_thread, NULL, &start_responder, NULL);
}

void resolve(uint16_t src, uint16_t dest) {
	struct ibv_send_wr wr, *bad_wr;
	struct ibv_recv_wr *resp_wr;
	struct ibv_mr *buf_mr = NULL;
	struct ibv_wc wc;
	struct ibv_sge sge;
	struct ibv_ah_attr ah_attr;
	struct ibv_ah *tmp_ah;

	buf_mr = ripc_alloc_recv_buf(sizeof(struct resolver_msg));

	if (!buf_mr) {
		ERROR("Failed to allocate multicast send mr");
		return;
	}

	DEBUG("Allocated multicast mr");
	DEBUG("Address: %p", buf_mr->addr);

	struct resolver_msg *msg = (struct resolver_msg *)buf_mr->addr;

	msg->type = RIPC_MSG_RESOLVE_REQ;
	msg->dest_service_id = dest;
	msg->src_service_id = src;
	msg->lid = context.lid;
	pthread_mutex_lock(&services_mutex);
	msg->service_qpn = context.services[src]->qp->qp_num;
	pthread_mutex_unlock(&services_mutex);
	msg->response_qpn = unicast_qp->qp_num;
	msg->resolver_qpn = mcast_qp->qp_num;

	sge.addr = (uint64_t)msg;
	sge.length = sizeof(struct resolver_msg);
	sge.lkey = buf_mr->lkey;

	wr.next = NULL;
	wr.num_sge = 1;
	wr.opcode = IBV_WR_SEND;
	wr.send_flags = IBV_SEND_SIGNALED;
	wr.sg_list = &sge;
	wr.wr_id = 0xdeadbeef;
	wr.wr.ud.ah = mcast_ah;
	wr.wr.ud.remote_qkey = 0xffff;
	wr.wr.ud.remote_qpn = 0xffffff;

	DEBUG("Posting multicast send request");

	ibv_post_send(mcast_qp, &wr, &bad_wr);

	while ( ! ibv_poll_cq(mcast_send_cq, 1, &wc)) { /* wait */ }

	ripc_buf_free(msg);

	DEBUG("Successfully sent multicast request, now waiting for reply on qp %u",
			unicast_qp->qp_num);

	/*
	 * Processing of the request shouldn't take long, so we just spin.
	 * TODO: Implement some sort of timeout!
	 */
	while ( ! ibv_poll_cq(unicast_recv_cq, 1, &wc)) { /* wait */ }

	post_new_recv_buf(unicast_qp);

	resp_wr = (struct ibv_recv_wr *)wc.wr_id;
	msg = (struct resolver_msg *)(resp_wr->sg_list->addr + 40);

	assert(msg->type == RIPC_MSG_RESOLVE_REPLY);
	assert(msg->dest_service_id == dest);

	DEBUG("Received message: from service: %u, for service: %u, from qpn: %u, from lid: %u, response to: %u",
			msg->src_service_id,
			msg->dest_service_id,
			msg->service_qpn,
			msg->lid,
			msg->response_qpn
			);

	 //got the info we wanted, now feed it to the cache
	ah_attr.dlid = msg->lid;
	ah_attr.port_num = 1;
	tmp_ah = ibv_create_ah(context.pd, &ah_attr);

	pthread_mutex_lock(&remotes_mutex);

	if (!context.remotes[dest]) {
		context.remotes[dest] = malloc(sizeof(struct remote_context));
		bzero(context.remotes[dest], sizeof(struct remote_context));
		context.remotes[dest]->state = RIPC_RDMA_DISCONNECTED;
	}
	context.remotes[dest]->ah = tmp_ah;
	context.remotes[dest]->qp_num = msg->service_qpn;
	context.remotes[dest]->resolver_qp = msg->resolver_qpn;

	pthread_mutex_unlock(&remotes_mutex);
}

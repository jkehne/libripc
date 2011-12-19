#include "resources.h"

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

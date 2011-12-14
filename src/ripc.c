#include <time.h>
#include <unistd.h>
#include "ripc.h"
#include "common.h"
#include "memory.h"

struct library_context context;

bool init(void) {
	if (context.device_context != NULL)
		return true;

	srand(time(NULL));
	struct ibv_device **sys_devices = ibv_get_device_list(NULL);
	struct ibv_context *device_context = NULL;
	uint32_t i;
	context.device_context = NULL;

	for (i = 0; i < UINT16_MAX; ++i) {
		context.services[i] = NULL;
		context.remotes[i] = NULL;
	}

	while (*sys_devices != NULL) {
		DEBUG("Opening device: %s", ibv_get_device_name(*sys_devices));
		device_context = ibv_open_device(*sys_devices);

		if (device_context == NULL) {
			ERROR("Failed to open device %s, trying next device",
					ibv_get_device_name(*sys_devices));
			sys_devices++;
		} else {
			context.device_context = device_context;
			DEBUG("Successfully opened %s", ibv_get_device_name(*sys_devices));
			break;
		}
	}

	if (context.device_context == NULL) {
		panic("No more devices available!");
	}

	context.pd = ibv_alloc_pd(context.device_context);
	if (context.pd == NULL) {
		panic("Failed to allocate protection domain!");
	} else {
		DEBUG("Allocated protection domain: %u", context.pd->handle);
	}

#ifdef HAVE_DEBUG
	struct ibv_device_attr device_attr;
	ibv_query_device(context.device_context,&device_attr);
	DEBUG("Device %s has %d physical ports",
			ibv_get_device_name(*sys_devices),
			device_attr.phys_port_cnt);
	int j;
	struct ibv_port_attr port_attr;
	union ibv_gid gid;
	for (i = 0; i < device_attr.phys_port_cnt; ++i) {
		ibv_query_port(context.device_context,i,&port_attr);
		DEBUG("Port %d: Found LID %u", i, port_attr.lid);
		DEBUG("Port %d has %d GIDs", i, port_attr.gid_tbl_len);
		DEBUG("Port %d's maximum message size is %u", i, port_attr.max_msg_sz);
//		for (j = 0; j < port_attr.gid_tbl_len; ++j) {
//			ibv_query_gid(context.device_context, i, j, &gid);
//			DEBUG("Port %d: Found GID %d: %016lx:%016lx",
//					i,
//					j,
//					gid.global.subnet_prefix,
//					gid.global.interface_id);
//		}
	}
#endif

	return true;
}

uint16_t ripc_register_random_service_id(void) {

	init();

	uint16_t service_id;

	do { //try to find a free service id
		service_id = rand() % UINT16_MAX;
	} while (ripc_register_service_id(service_id) == false);

	return service_id;
}

uint8_t ripc_register_service_id(int service_id) {

	init();
	DEBUG("Allocating service ID %u", service_id);

	struct service_id *service_context;
	uint32_t i;

	if (context.services[service_id] != NULL)
		return false; //already allocated

	context.services[service_id] =
		(struct service_id *)malloc(sizeof(struct service_id));
	memset(context.services[service_id],0,sizeof(struct service_id));
	service_context = context.services[service_id];

	service_context->number = service_id;

	service_context->cchannel = ibv_create_comp_channel(context.device_context);
	if (service_context->cchannel == NULL) {
		ERROR("Failed to allocate completion event channel!");
		free(service_context);
		return false;
	} else {
		DEBUG("Allocated completion event channel");
	}

	service_context->recv_cq = ibv_create_cq(
			context.device_context,
			100,
			NULL,
			service_context->cchannel,
			0);
	if (service_context->recv_cq == NULL) {
		ERROR("Failed to allocate receive completion queue!");
		ibv_destroy_comp_channel(service_context->cchannel);
		free(service_context);
		return false;
	} else {
		DEBUG("Allocated receive completion queue: %u", service_context->recv_cq->handle);
	}

	service_context->send_cq = ibv_create_cq(
			context.device_context,
			100,
			NULL,
			NULL,
			0);
	if (service_context->send_cq == NULL) {
		ERROR("Failed to allocate send completion queue!");
		ibv_destroy_comp_channel(service_context->cchannel);
		ibv_destroy_cq(service_context->recv_cq);
		free(service_context);
		return false;
	} else {
		DEBUG("Allocated send completion queue: %u", service_context->send_cq->handle);
	}

	ibv_req_notify_cq(service_context->recv_cq, 0);

	struct ibv_qp_init_attr init_attr = {
		.send_cq = service_context->send_cq,
		.recv_cq = service_context->recv_cq,
		.cap     = {
			.max_send_wr  = NUM_RECV_BUFFERS * 200,
			.max_recv_wr  = NUM_RECV_BUFFERS * 200,
			.max_send_sge = 10,
			.max_recv_sge = 10
		},
		.qp_type = IBV_QPT_UD,
		.sq_sig_all = 0
	};
	service_context->qp = ibv_create_qp(
			context.pd,
			&init_attr);
	if (service_context->qp == NULL) {
		ERROR("Failed to allocate queue pair!");
		ibv_destroy_cq(service_context->recv_cq);
		ibv_destroy_comp_channel(service_context->cchannel);
		free(service_context);
		return false;
	} else {
		DEBUG("Allocated queue pair: %u", service_context->qp->qp_num);
	}

	struct ibv_qp_attr attr;
	attr.qp_state = IBV_QPS_INIT;
	attr.pkey_index = 0;
	attr.port_num = 1;
	attr.qkey = service_id;

	if (ibv_modify_qp(service_context->qp,
			&attr,
			IBV_QP_STATE		|
			IBV_QP_PKEY_INDEX	|
			IBV_QP_PORT			|
			IBV_QP_QKEY			)) {
		ERROR("Failed to modify QP state to INIT");
		ibv_destroy_qp(service_context->qp);
		ibv_destroy_cq(service_context->recv_cq);
		ibv_destroy_comp_channel(service_context->cchannel);
		free(service_context);
		return false;
	} else {
		DEBUG("Modified state of QP %u to INIT",service_context->qp->qp_num);
	}

	attr.qp_state = IBV_QPS_RTR;
	if (ibv_modify_qp(service_context->qp, &attr, IBV_QP_STATE)) {
		ERROR("Failed to modify QP state to RTR");
		ibv_destroy_qp(service_context->qp);
		ibv_destroy_cq(service_context->recv_cq);
		ibv_destroy_comp_channel(service_context->cchannel);
		free(service_context);
		return false;
	} else {
		DEBUG("Modified state of QP %u to RTR",service_context->qp->qp_num);
	}

	attr.qp_state = IBV_QPS_RTS;
	attr.sq_psn = 0;
	if (ibv_modify_qp(service_context->qp, &attr, IBV_QP_STATE | IBV_QP_SQ_PSN)) {
		ERROR("Failed to modify QP state to RTS");
		ibv_destroy_qp(service_context->qp);
		ibv_destroy_cq(service_context->recv_cq);
		ibv_destroy_comp_channel(service_context->cchannel);
		free(service_context);
		return false;
	} else {
		DEBUG("Modified state of QP %u to RTS",service_context->qp->qp_num);
	}

#ifdef HAVE_DEBUG
	ibv_query_qp(service_context->qp,&attr,~0,&init_attr);
	DEBUG("qkey of QP %u is %d", service_context->qp->qp_num, attr.qkey);
#endif

	for (i = 0; i < NUM_RECV_BUFFERS; ++i) {
		post_new_recv_buf(service_context);
	}

#ifndef HAVE_DEBUG
	printf("qp_num is %u\n", service_context->qp->qp_num);
#endif

	return true;
}

uint8_t
ripc_send_short(
		uint16_t src,
		uint16_t dest,
		void **buf,
		uint32_t *length,
		uint32_t num_items) {

	uint32_t i, total_length = 0;

	for (i = 0; i < num_items; ++i)
		total_length += length[i];
	if (total_length > (
			RECV_BUF_SIZE
			- sizeof(struct msg_header)
			- sizeof(struct short_header) * num_items
			))
		return -1; //probably won't fit at receiving end either


	//build packet header
	struct ibv_mr *header_mr =
			ripc_alloc_recv_buf(
					sizeof(struct msg_header)
					+ sizeof(struct short_header) * num_items
					);
	struct msg_header *hdr = (struct msg_header *)header_mr->addr;
	struct short_header *msg =
			(struct short_header *)((uint64_t)hdr + sizeof(struct msg_header));

	hdr->type = RIPC_MSG_SEND;
	hdr->from = src;
	hdr->to = dest;
	hdr->short_words = num_items;
	hdr->long_words = 0;
	hdr->return_bufs = 0;

	struct ibv_sge sge[num_items + 1]; //+1 for header
	sge[0].addr = (uint64_t)header_mr->addr;
	sge[0].length = sizeof(struct msg_header)
							+ sizeof(struct short_header) * num_items;
	sge[0].lkey = header_mr->lkey;

	uint32_t offset =
			40 //skip GRH
			+ sizeof(struct msg_header)
			+ sizeof(struct short_header) * num_items;

	for (i = 0; i < num_items; ++i) {

		DEBUG("First message: offset %#x, length %u", offset, length[i]);
		msg[i].offset = offset;
		msg[i].size = length[i];

		offset += length[i]; //offset of next message item

		//make sure send buffers are registered with the hardware
		struct ibv_mr *mr = used_buf_list_get(buf[i]);
		void *tmp_buf;

		if (!mr) { //not registered yet
			DEBUG("mr not found in cache, creating new one");
			mr = ripc_alloc_recv_buf(length[i]);
			tmp_buf = mr->addr;
			memcpy(tmp_buf,buf[i],length[i]);
		} else {
			DEBUG("Found mr in cache!");
			used_buf_list_add(mr);
			tmp_buf = buf[i];
		}

		assert(mr);
		assert(mr->length >= length[i]); //the hardware won't allow it anyway

		sge[i + 1].addr = (uint64_t)tmp_buf;
		sge[i + 1].length = length[i];
		sge[i + 1].lkey = mr->lkey;
	}

	struct ibv_ah *ah; //where to send it

	if (context.remotes[dest] && context.remotes[dest]->ah)
		ah = context.remotes[dest]->ah;
	else {
		DEBUG("Creating new address handle for remote %u", dest);
		struct ibv_ah_attr ah_attr;
		ah_attr.dlid = dest;
		ah_attr.port_num = 1; //TODO: Make this dynamic
		ah = ibv_create_ah(context.pd,&ah_attr);
		assert(ah);
		if (! context.remotes[dest])
			context.remotes[dest] = malloc(sizeof(struct remote_context));
		assert (context.remotes[dest]);
		context.remotes[dest]->ah = ah;
		context.remotes[dest]->qp_num = 0; //probably invalid anyway if the ah has changed
	}

	struct ibv_send_wr wr;
	wr.next = NULL;
	wr.opcode = IBV_WR_SEND;
	wr.num_sge = num_items + 1;
	wr.sg_list = &sge;
	wr.wr_id = 0xdeadbeef; //TODO: Make this a counter?
	wr.wr.ud.ah = ah;
	wr.wr.ud.remote_qkey = (uint32_t)dest;
	wr.wr.ud.remote_qpn =
			context.remotes[dest]->qp_num ?
					context.remotes[dest]->qp_num :
					5505098; //TODO: Make this dynamic
//#ifdef HAVE_DEBUG
	wr.send_flags = IBV_SEND_SIGNALED;
//#else
//	wr.send_flags = IBV_SEND_INLINE;
//#endif

	DEBUG("Sending message containing %u items to lid %u, qpn %u using qkey %d",
			wr.num_sge,
			dest,
			wr.wr.ud.remote_qpn,
			wr.wr.ud.remote_qkey);
#ifdef HAVE_DEBUG
	for (i = 0; i < wr.num_sge; ++i) {
	ERROR("Item %u: address: %p, length %u",
			i,
			wr.sg_list[i].addr,
			wr.sg_list[i].length);
	}
#endif

	struct ibv_send_wr *bad_wr = NULL;

	int ret = ibv_post_send(context.services[src]->qp, &wr, &bad_wr);

	if (bad_wr) {
		ERROR("Failed to post send! Return code: %d", ret);
		ERROR("QP state: %d", context.services[src]->qp->state);
		return ret;
	} else {
		DEBUG("Successfully posted send!");
	}


//#ifdef HAVE_DEBUG
	struct ibv_wc wc;
	while (!(ibv_poll_cq(context.services[src]->send_cq, 1, &wc))); //polling is probably faster here
	//ibv_ack_cq_events(context.services[src]->recv_cq, 1);
	DEBUG("received completion message!");
	if (wc.status) {
		ERROR("Send result: %d", wc.status);
	}
	DEBUG("Result: %d", wc.status);
//#endif

	ripc_buf_free(hdr);

	return wc.status;
}

uint8_t
ripc_send_long(
		uint16_t src,
		uint16_t dest,
		void *buf,
		uint32_t length) {

	return 0;
}

uint8_t
ripc_receive(
		uint16_t service_id,
		void ***short_items,
		void ***long_items) {

	struct ibv_wc wc;
	void *ctx;
	struct ibv_cq *cq;
	uint32_t i;

	restart:
	do {
		ibv_get_cq_event(context.services[service_id]->cchannel,
		&cq,
		&ctx);

		assert(cq == context.services[service_id]->recv_cq);

		ibv_ack_cq_events(context.services[service_id]->recv_cq, 1);
		ibv_req_notify_cq(context.services[service_id]->recv_cq, 0);

	} while (!(ibv_poll_cq(context.services[service_id]->recv_cq, 1, &wc)));

	DEBUG("received!");

	post_new_recv_buf(context.services[service_id]);

	struct ibv_recv_wr *wr = (struct ibv_recv_wr *)(wc.wr_id);
	struct msg_header *hdr = (struct msg_header *)(wr->sg_list->addr + 40);

	if (hdr->type != RIPC_MSG_SEND) {
		free(wr->sg_list);
		free(wr);
		ERROR("Spurious message, restarting");
		goto restart;
	}

	DEBUG("Message type is %#x", hdr->type);

	struct short_header *msg =
			(struct short_header *)(wr->sg_list->addr
					+ 40 //skip GRH
					+ sizeof(struct msg_header));

	assert(hdr->to == service_id);

	//cache remote address handle if we don't have it already
	if (( ! context.remotes[hdr->from]) ||
			( ! context.remotes[hdr->from]->ah)) {
		DEBUG("Caching remote address handle for remote %u", hdr->from);

		if ( ! context.remotes[hdr->from])
			context.remotes[hdr->from] = malloc(sizeof(struct remote_context));

		assert(context.remotes[hdr->from]);

		context.remotes[hdr->from]->ah =
				ibv_create_ah_from_wc(context.pd, &wc, NULL, 1);

		//not conditional as we assume when the ah needs updating, so does the qp number
		context.remotes[hdr->from]->qp_num = wc.src_qp;
	}

	*short_items = malloc(sizeof(void *) * hdr->short_words);
	for (i = 0; i < hdr->short_words; ++i) {
		(*short_items)[i] = (void *)(wr->sg_list->addr + msg[i].offset);
	}

	free(wr->sg_list);
	free(wr);

	return hdr->short_words + hdr->long_words;
}
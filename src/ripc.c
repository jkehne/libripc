#include <time.h>
#include "ripc.h"
#include "common.h"
#include "memory.h"

struct library_context context;
struct ibv_ah *ah_cache[UINT16_MAX];

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
		ah_cache[i] = NULL;
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

	if (context.services[service_id] == NULL) {
		context.services[service_id] =
			(struct service_id *)malloc(sizeof(struct service_id));
		memset(context.services[service_id],0,sizeof(struct service_id));
	}
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

	service_context->cq = ibv_create_cq(
			context.device_context,
			100,
			NULL,
			service_context->cchannel,
			0);
	if (service_context->cq == NULL) {
		ERROR("Failed to allocate completion queue!");
		ibv_destroy_comp_channel(service_context->cchannel);
		free(service_context);
		return false;
	} else {
		DEBUG("Allocated completion queue: %u", service_context->cq->handle);
	}

	ibv_req_notify_cq(service_context->cq, 0);

	struct ibv_qp_init_attr init_attr = {
		.send_cq = service_context->cq,
		.recv_cq = service_context->cq,
		.cap     = {
			.max_send_wr  = NUM_RECV_BUFFERS,
			.max_recv_wr  = NUM_RECV_BUFFERS,
			.max_send_sge = 1,
			.max_recv_sge = 1
		},
		.qp_type = IBV_QPT_UD
	};
	service_context->qp = ibv_create_qp(
			context.pd,
			&init_attr);
	if (service_context->qp == NULL) {
		ERROR("Failed to allocate queue pair!");
		ibv_destroy_cq(service_context->cq);
		ibv_destroy_comp_channel(service_context->cchannel);
		free(service_context);
		return false;
	} else {
		DEBUG("Allocated queue pair: %u", service_context->qp->handle);
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
		ibv_destroy_cq(service_context->cq);
		ibv_destroy_comp_channel(service_context->cchannel);
		free(service_context);
		return false;
	} else {
		DEBUG("Modified state of QP %u to INIT",service_context->qp->handle);
	}

	attr.qp_state = IBV_QPS_RTR;
	if (ibv_modify_qp(service_context->qp, &attr, IBV_QP_STATE)) {
		ERROR("Failed to modify QP state to RTR");
		ibv_destroy_qp(service_context->qp);
		ibv_destroy_cq(service_context->cq);
		ibv_destroy_comp_channel(service_context->cchannel);
		free(service_context);
		return false;
	} else {
		DEBUG("Modified state of QP %u to RTR",service_context->qp->handle);
	}

	attr.qp_state = IBV_QPS_RTS;
	attr.sq_psn = 0;
	if (ibv_modify_qp(service_context->qp, &attr, IBV_QP_STATE | IBV_QP_SQ_PSN)) {
		ERROR("Failed to modify QP state to RTS");
		ibv_destroy_qp(service_context->qp);
		ibv_destroy_cq(service_context->cq);
		ibv_destroy_comp_channel(service_context->cchannel);
		free(service_context);
		return false;
	} else {
		DEBUG("Modified state of QP %u to RTS",service_context->qp->handle);
	}

	struct ibv_mr *mr = ripc_alloc_recv_buf(RECV_BUF_SIZE * NUM_RECV_BUFFERS);
	struct ibv_sge *list = malloc(sizeof(struct ibv_sge));
	struct ibv_recv_wr *wr, *bad_wr;
	wr = malloc(sizeof(struct ibv_recv_wr));
	uint64_t ptr = (uint64_t)mr->addr;

	for (i = 0; i < NUM_RECV_BUFFERS; ++i) {
		list->addr = ptr + i * RECV_BUF_SIZE;
		list->length = RECV_BUF_SIZE;
		list->lkey = mr->lkey;

		wr->wr_id = (uint64_t)wr;
		wr->sg_list = list;
		wr->num_sge = 1;
		wr->next = NULL;

		if (ibv_post_recv(service_context->qp, wr, &bad_wr)) {
			ERROR("Failed to post receive item to QP %u!", service_context->qp->handle);
		} else {
			DEBUG("Posted receive buffer at address %p to QP %u",
					ptr + i * RECV_BUF_SIZE,
					service_context->qp->handle);
		}
	}

	return true;
}

uint8_t ripc_send_short(uint16_t src, uint16_t dest, void *buf, uint32_t length) {
	if (length > RECV_BUF_SIZE) //TODO: minus header size
		return -1;

	struct ibv_mr *mr = used_buf_list_get(buf);

	if (!mr) { //not registered yet
		void *tmp_buf = valloc(length); //align, just in case
		memcpy(tmp_buf,buf,length);
		mr = ripc_buf_register(tmp_buf, length);
	}

	assert(mr);
	assert(mr->length >= length); //the hardware won't allow this anyway

	struct ibv_sge sge; //what to send
	sge.addr = (uint64_t)buf;
	sge.length = length;
	sge.lkey = mr->lkey;

	struct ibv_ah *ah = ah_cache[dest]; //where to send it
	if (!ah) {
		struct ibv_ah_attr ah_attr;
		ah_attr.dlid = dest;
		ah_attr.port_num = 1; //TODO: Make this dynamic
		ah = ibv_create_ah(context.pd,&ah_attr);
		assert(ah);
		ah_cache[dest] = ah;
	}
	struct ibv_send_wr wr;
	wr.next = NULL;
	wr.opcode = IBV_WR_SEND;
	wr.num_sge = 1;
	wr.sg_list = &sge;
	wr.wr_id = 0xdeadbeef; //TODO: Make this a counter?
	wr.wr.ud.ah = ah;
	wr.wr.ud.remote_qkey = dest;
	wr.wr.ud.remote_qpn = 1; //TODO: Make this dynamic

	struct ibv_send_wr **bad_wr = NULL;

	ibv_post_send(context.services[src]->qp, &wr, bad_wr);

	return bad_wr ? -1 : 0;
}

uint8_t ripc_send_long(uint16_t src, uint16_t dest, void *buf, uint32_t length) {

}

uint8_t ripc_receive(uint16_t service_id, void *short_items[], void *long_items[]) {
	struct ibv_wc *wc;
	void *ctx;
	struct ibv_cq *cq;

	ibv_get_cq_event(context.services[service_id]->cchannel,
			&cq,
			ctx);

	assert(cq == context.services[service_id]->cq);

	ibv_ack_cq_events(context.services[service_id]->cq, 1);
	ibv_req_notify_cq(context.services[service_id]->cq, 0);

	ibv_poll_cq(context.services[service_id]->cq, 1, wc);

	struct ibv_recv_wr *wr = (struct ibv_recv_wr *)(wc->wr_id);
	short_items = malloc(sizeof(void *));
	short_items[0] = (void *)wr->sg_list->addr;
}

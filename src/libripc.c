#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ripc.h"
#include "common.h"

struct library_context context;

bool init(void) {
	if (context.device_context != NULL)
		return true;
	srand(time(NULL));
	struct ibv_device **sys_devices = ibv_get_device_list(NULL);
	struct ibv_context *device_context = NULL;
	uint32_t i;
	context.device_context = NULL;
	for (i = 0; i < UINT16_MAX; ++i)
		context.services[i] = NULL;
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

	context.cchannel = ibv_create_comp_channel(context.device_context);
	if (context.cchannel == NULL) {
		panic("Failed to allocate completion event channel!");
	} else {
		DEBUG("Allocated completion event channel");
	}
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

	if (context.services[service_id] != NULL)
		return false; //already allocated

	context.services[service_id] =
			(struct service_id *)malloc(sizeof(struct service_id));
	service_context = context.services[service_id];

	service_context->number = service_id;
	service_context->cq = ibv_create_cq(
			context.device_context,
			100,
			NULL,
			NULL,
			0);
	DEBUG("Allocated completion queue: %u", service_context->cq->handle);

	struct ibv_qp_init_attr attr = {
		.send_cq = service_context->cq,
		.recv_cq = service_context->cq,
		.cap     = {
			.max_send_wr  = 10,
			.max_recv_wr  = 10,
			.max_send_sge = 1,
			.max_recv_sge = 1
		},
		.qp_type = IBV_QPT_UD
	};
	service_context->qp = ibv_create_qp(
			context.pd,
			&attr);
	DEBUG("Allocated queue pair: %u", service_context->qp->handle);
	return true;
}

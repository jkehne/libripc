#ifndef RESOURCES_H_
#define RESOURCES_H_

#include "config.h"
#include "common.h"
#include "ripc.h"

void alloc_queue_state(
		struct ibv_comp_channel **cchannel,
		struct ibv_cq **send_cq,
		struct ibv_cq **recv_cq,
		struct ibv_qp **qp,
		uint16_t service_id
		);

#endif /* RESOURCES_H_ */

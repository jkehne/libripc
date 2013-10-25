#ifndef NETARCH_CONTEXT_H_
#define NETARCH_CONTEXT_H_

#include <common.h>

/**
 * Record to be stored in the central infrastructure.
 *
 * Data contained herein will be used to create
 * instances of netarch_sending.
 *
 * TODO: Support different endianness.
 */
struct netarch_address_record {
	uint16_t lid; // Via context.na.lid (netarch_init())
	uint32_t qp_num; // Via qp_create()->num
};

// TODO: Reorganize or rename the following structs.
//       One of them is passive locally and only used for addressing
//       (netarch_sending), the other is our local, active
//       process that sends and receives messages. -- awaidler, 2013-10-25

/**
 * Contains information to address a certain process.
 */
struct netarch_sending {
	uint32_t qp_num;
	struct ibv_cq *cq;
	struct ibv_ah *ah;
	struct netarch_rdma_context *rdma[UINT16_MAX];
};

/**
 * Contains information to send messages from the local hardware
 * as a certain process and receive messages send to us.
 */
struct netarch_receiving {
	struct ibv_qp *qp; // For newly created caps (qpn, recv_buf).
	struct ibv_cq *cq; // For polling for receive event.
	struct ibv_comp_channel *cchan; // For reading receive events, see man ibv_create_comp_channel().
	bool no_cchan;
};

#endif // NETARCH_CONTEXT_H_

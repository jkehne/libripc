#ifndef RESOURCES_H_
#include "../resources.h"
#endif

#ifndef __INFINIBAND__RESOURCES_H__
#define __INFINIBAND__RESOURCES_H__

void dump_qp_state(struct ibv_qp *qp);

void alloc_queue_state(
		struct ibv_comp_channel **cchannel,
		struct ibv_cq **send_cq,
		struct ibv_cq **recv_cq,
		struct ibv_qp **qp,
		uint16_t service_id
		);

void create_rdma_connection(
               uint16_t src,
               uint16_t dest
               );

extern struct ibv_cq *rdma_send_cq, *rdma_recv_cq;
extern struct ibv_qp *rdma_qp;

#endif /* !__INFINIBAND__RESOURCES_H__ */

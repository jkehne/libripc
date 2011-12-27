#ifndef RESOURCES_H_
#define RESOURCES_H_

#include "config.h"
#include "common.h"
#include "ripc.h"

struct rdma_connect_msg {
       enum msg_type type;
       uint16_t dest_service_id;
       uint16_t src_service_id;
       uint16_t lid;
       uint32_t qpn;
       uint32_t psn;
       uint32_t response_qpn;
};

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
extern pthread_mutex_t rdma_connect_mutex;

#endif /* RESOURCES_H_ */

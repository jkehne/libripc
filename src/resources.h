#ifndef RESOURCES_H_
#define RESOURCES_H_

#include <config.h>
#include <common.h>
#include <ripc.h>
#ifdef NETARCH_INFINIBAND
#include <infiniband/resources.h>
#endif
#ifdef NETARCH_BGP
#include <bgp/resources.h>
#endif 
#ifdef NETARCH_LOCAL
#include <local/resources.h>
#endif 

struct rdma_connect_msg {
       enum msg_type type;
       uint16_t dest_service_id;
       uint16_t src_service_id;
       uint16_t lid;
       uint32_t qpn;
       uint32_t psn;
       uint32_t response_qpn;
};
extern pthread_mutex_t rdma_connect_mutex;
extern struct service_id rdma_service_id;

void alloc_queue_state(struct service_id *service_id);

#endif /* RESOURCES_H_ */

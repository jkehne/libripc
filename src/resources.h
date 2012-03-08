#ifndef RESOURCES_H_
#define RESOURCES_H_

#include <config.h>
#include <common.h>
#include <ripc.h>
#ifdef ENABLE_INFINIBAND
#include <infiniband/resources.h>
#endif
#ifdef ENABLE_BGP
#include <bgp/resources.h>
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

#endif /* RESOURCES_H_ */

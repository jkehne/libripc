#ifndef RESOURCES_H_
#include "../resources.h"
#endif

#ifndef __INFINIBAND__RESOURCES_H__
#define __INFINIBAND__RESOURCES_H__

void dump_qp_state(struct ibv_qp *qp);

void create_rdma_connection(
               uint16_t src,
               uint16_t dest
               );

#endif /* !__INFINIBAND__RESOURCES_H__ */

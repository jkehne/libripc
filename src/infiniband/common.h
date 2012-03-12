#ifndef COMMON_H_
#include "../common.h"
#endif

#ifndef __INFINIBAND_COMMON_H__
#define __INFINIBAND_COMMON_H__

#include <infiniband/verbs.h>

struct netarch_service_id {
        bool no_cchannel;
	struct ibv_cq *send_cq;
	struct ibv_cq *recv_cq;
	struct ibv_qp *qp;
	struct ibv_comp_channel *cchannel;
};

struct netarch_remote_context {
	struct ibv_ah *ah;
	struct ibv_qp *rdma_qp;
	struct ibv_cq *rdma_send_cq;
	struct ibv_cq *rdma_recv_cq;
	struct ibv_comp_channel *rdma_cchannel;
};

struct netarch_library_context {
	struct ibv_context *device_context;
	struct ibv_pd *pd;
	uint16_t lid;
};

#endif /* !__INFINIBAND_COMMON_H__ */

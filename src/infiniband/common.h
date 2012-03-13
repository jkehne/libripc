/*  Copyright 2011, 2012 Jens Kehne
 *  Copyright 2012 Jan Stoess, Karlsruhe Institute of Technology
 *
 *  LibRIPC is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License as published by the
 *  Free Software Foundation, either version 2.1 of the License, or (at your
 *  option) any later version.
 *
 *  LibRIPC is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with libRIPC.  If not, see <http://www.gnu.org/licenses/>.
 */
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

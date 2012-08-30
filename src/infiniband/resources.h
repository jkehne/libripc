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
#ifndef RESOURCES_H_
#include "../resources.h"
#endif

#ifndef __INFINIBAND__RESOURCES_H__
#define __INFINIBAND__RESOURCES_H__

#include <infiniband/multicast_resources.h>

struct netarch_rdma_connect_msg {
	uint16_t lid;
	uint32_t qpn;
	uint32_t psn;
	uint32_t response_qpn;
};

void dump_qp_state(struct ibv_qp *qp);
void dump_wr(struct ibv_send_wr wr, bool ud);
void dump_wc(struct ibv_wc wc);

void create_rdma_connection(
               uint16_t src,
               uint16_t dest
               );

#endif /* !__INFINIBAND__RESOURCES_H__ */

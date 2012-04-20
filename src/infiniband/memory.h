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
#ifndef MEMORY_H_
#include "../memory.h"
#endif

#ifndef __INFINIBAND__MEMORY_H__
#define __INFINIBAND__MEMORY_H__

typedef struct ibv_mr *netarch_mem_buf_t;

#define INVALID_NETARCH_MEM_BUF NULL

void post_new_recv_buf(struct ibv_qp *qp);

#endif /* !__INFINIBAND__MEMORY_H__ */

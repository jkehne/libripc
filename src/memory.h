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
#define MEMORY_H_

#include <sys/types.h>
#include <pthread.h>
#include <common.h>

struct mem_buf_list {
	struct ibv_mr *mr;
	struct mem_buf_list *next;
	void *base; //these two members are for receive windows
	size_t size;
};

extern pthread_mutex_t used_list_mutex, free_list_mutex, recv_window_mutex;

struct ibv_mr *used_buf_list_get(void *addr);
void used_buf_list_add(struct ibv_mr *item);

struct ibv_mr *free_buf_list_get(size_t size);
void free_buf_list_add(struct ibv_mr *item);

void *recv_window_list_get(size_t size);
void recv_window_list_add(struct ibv_mr *item, void *base, size_t size);

struct ibv_mr *return_buf_list_get(uint16_t remote, size_t size);
void return_buf_list_add(uint16_t remote, struct ibv_mr *item);

#ifdef NETARCH_INFINIBAND
void post_new_recv_buf(struct ibv_qp *qp);
#endif

struct ibv_mr *ripc_alloc_recv_buf(size_t size);

#endif /* MEMORY_H_ */

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

#ifdef NETARCH_INFINIBAND
#include <infiniband/memory.h>
#endif
#ifdef NETARCH_BGP
#include <bgp/memory.h>
#endif 
#ifdef NETARCH_LOCAL
#include <local/memory.h>
#endif 


typedef struct mem_buf {
        uint64_t addr; // mirrors netarch settings
        size_t size;
        void *rcv_addr; //these two members are for receive windows
	size_t rcv_size;
	netarch_mem_buf_t na;
} mem_buf_t;


typedef struct mem_buf_list {
	struct mem_buf_list *next;
	struct mem_buf buf;
} mem_buf_list_t;

extern pthread_mutex_t used_list_mutex, free_list_mutex, recv_window_mutex;
static const mem_buf_t invalid_mem_buf = { 0, -1, NULL, -1, INVALID_NETARCH_MEM_BUF };
        
mem_buf_t used_buf_list_get(void *addr);
void used_buf_list_add(mem_buf_t mem_buf);

mem_buf_t free_buf_list_get(size_t size);
void free_buf_list_add(mem_buf_t mem_buf);

void *recv_window_list_get(size_t size);
void recv_window_list_add(mem_buf_t mem_buf);


mem_buf_t return_buf_list_get(uint16_t remote, size_t size);
void return_buf_list_add(uint16_t remote, mem_buf_t mem_buf);

mem_buf_t ripc_alloc_recv_buf(size_t size);

#endif /* MEMORY_H_ */

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
#include "ripc.h"
#include "common.h"
#include "memory.h"
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

struct mem_buf_list *used_buffers = NULL;
struct mem_buf_list *available_buffers = NULL;
struct mem_buf_list *available_buffers_tail = NULL;
struct mem_buf_list *receive_windows = NULL;
struct mem_buf_list *receive_windows_tail = NULL;

pthread_mutex_t used_list_mutex, free_list_mutex, recv_window_mutex;

void used_buf_list_add(mem_buf_t mem_buf) {
	struct mem_buf_list *container = malloc(sizeof(struct mem_buf_list));
	assert(container);
	memset(container, 0, sizeof(struct mem_buf_list));

	container->buf = mem_buf;
	pthread_mutex_lock(&used_list_mutex);

	if (used_buffers == NULL) {
		container->next = NULL;
		used_buffers = container;
	} else {
		container->next = used_buffers;
		used_buffers = container;
	}

	pthread_mutex_unlock(&used_list_mutex);
	return;
}

mem_buf_t used_buf_list_get(void *addr) {
	pthread_mutex_lock(&used_list_mutex);

	struct mem_buf_list *ptr = used_buffers;
	struct mem_buf_list *prev = NULL;

	while(ptr) {
		if (((uint64_t)addr >= ptr->buf.addr)
				&& ((uint64_t)addr < ptr->buf.addr + ptr->buf.size)) {
			if (prev)
				prev->next = ptr->next;
			else //first element in list
				used_buffers = ptr->next;
				//NOTE: If ptr is the only element, then ptr->next is NULL
			pthread_mutex_unlock(&used_list_mutex);
			mem_buf_t ret = ptr->buf;
			free(ptr);
			return ret;
		}
		prev = ptr;
		ptr = ptr->next;
	}
	pthread_mutex_unlock(&used_list_mutex);
	return invalid_mem_buf; //not found
}

void free_buf_list_add(mem_buf_t mem_buf) {
	struct mem_buf_list *container = malloc(sizeof(struct mem_buf_list));
	assert(container);
	memset(container, 0, sizeof(struct mem_buf_list));

	container->buf = mem_buf;
	container->next = NULL;

	pthread_mutex_lock(&free_list_mutex);

	if (available_buffers_tail == NULL) {
		available_buffers = container;
		available_buffers_tail = container;
	} else {
		available_buffers_tail->next = container;
		available_buffers_tail = container;
                
	}

	pthread_mutex_unlock(&free_list_mutex);
	return;
}

mem_buf_t free_buf_list_get(size_t size) {
	pthread_mutex_lock(&free_list_mutex);

	struct mem_buf_list *ptr = available_buffers;
	struct mem_buf_list *prev = NULL;

	while(ptr) {
		/*
		 * Accept the buffer if
		 * 1) it is bigger than the requested size (obviously)
		 * 2) it is smaller than twice the requested size.
		 * If we would leave out the second requirement, the library
		 * sometimes uses very large buffers for small payloads, and is
		 * then forced to allocate a new, large buffer soon after. The
		 * idea is to instead find a buffer of *approximately* the
		 * requested size.
		 * In practise, this doesn't change transfer time much, but it
		 * significantly reduces jitter!
		 */
		if ((ptr->buf.size >= size) &&
				(ptr->buf.size <= size * 2)) {
			if (prev)
				prev->next = ptr->next;
			else //first element in list
				available_buffers = ptr->next;
				//NOTE: If ptr is the only element, then ptr->next is NULL
			if (ptr == available_buffers_tail) {
				available_buffers_tail = prev;
				if (prev)
					prev->next = NULL;
			}
			pthread_mutex_unlock(&free_list_mutex);
			mem_buf_t ret = ptr->buf;
			free(ptr);
			return ret;
		}
		prev = ptr;
		ptr = ptr->next;
	}
	pthread_mutex_unlock(&free_list_mutex);
	return invalid_mem_buf; //not found
}

void recv_window_list_add(mem_buf_t mem_buf) {
	DEBUG("Registering receive window at address %p, size %zu", mem_buf.rcv_addr, mem_buf.rcv_size);

	struct mem_buf_list *container = malloc(sizeof(struct mem_buf_list));
	assert(container);
	memset(container, 0, sizeof(struct mem_buf_list));

	container->next = NULL;
 	container->buf = mem_buf;

	pthread_mutex_lock(&recv_window_mutex);

	if (receive_windows_tail == NULL) {
		receive_windows = container;
		receive_windows_tail = container;
	} else {
		receive_windows_tail->next = container;
		receive_windows_tail = container;
	}

	pthread_mutex_unlock(&recv_window_mutex);
	return;
}

void *recv_window_list_get(size_t size) {
	pthread_mutex_lock(&recv_window_mutex);

	struct mem_buf_list *ptr = receive_windows;
	struct mem_buf_list *prev = NULL;

	while(ptr) {
		size_t ptr_size = ptr->buf.rcv_size;
		DEBUG("Checking window list entry: entry size: %zu, requested size: %zu",
				ptr_size,
				size);
		if (ptr_size >= size) {
			if (prev)
				prev->next = ptr->next;
			else //first element in list
				receive_windows = ptr->next;
				//NOTE: If ptr is the only element, then ptr->next is NULL
			if (ptr == receive_windows_tail) {
				receive_windows_tail = prev;
				if (prev)
					prev->next = NULL;
			}
			pthread_mutex_unlock(&recv_window_mutex);
			void *ret = ptr->buf.rcv_addr;
			free(ptr);
			return ret;
		}
		prev = ptr;
		ptr = ptr->next;
	}
	pthread_mutex_unlock(&recv_window_mutex);
	return NULL; //not found
}

void return_buf_list_add(uint16_t remote, mem_buf_t mem_buf) {
	struct mem_buf_list *container = malloc(sizeof(struct mem_buf_list));
	assert(container);
	memset(container, 0, sizeof(struct mem_buf_list));

	container->buf = mem_buf;

	pthread_mutex_lock(&remotes_mutex);

	if (context.remotes[remote]->return_bufs == NULL) {
		container->next = NULL;
		context.remotes[remote]->return_bufs = container;
	} else {
		container->next = context.remotes[remote]->return_bufs;
		context.remotes[remote]->return_bufs = container;
	}

	pthread_mutex_unlock(&remotes_mutex);
	return;
}

mem_buf_t return_buf_list_get(uint16_t remote, size_t size) {
	pthread_mutex_lock(&remotes_mutex);

	struct mem_buf_list *ptr = context.remotes[remote]->return_bufs;
	struct mem_buf_list *prev = NULL;

	while(ptr) {
		if (ptr->buf.size >= size) {
			if (prev)
				prev->next = ptr->next;
			else //first element in list
				context.remotes[remote]->return_bufs = ptr->next;
				//NOTE: If ptr is the only element, then ptr->next is NULL
			pthread_mutex_unlock(&remotes_mutex);
			mem_buf_t ret = ptr->buf;
			free(ptr);
			return ret;
		}
		prev = ptr;
		ptr = ptr->next;
	}
	pthread_mutex_unlock(&remotes_mutex);
	return invalid_mem_buf; //not found
}

void *ripc_buf_alloc(size_t size) {
	return (void *) ripc_alloc_recv_buf(size).addr;
}

void ripc_buf_free(void *buf) {
	DEBUG("Putting buffer %p into free list", buf);
	mem_buf_t mem_buf =  used_buf_list_get(buf);
	if (mem_buf.size != -1)
                free_buf_list_add(mem_buf);
	else {
		DEBUG("Buffer not found!");
	}
}

uint8_t ripc_reg_recv_window(void *rcv_addr, size_t rcv_size) {
	mem_buf_t mem_buf;
	assert(rcv_size > 0);

	if (rcv_addr == NULL) { //the user wants us to specify the buffer
		DEBUG("No rcv_addr specified, allocating new buffer");
		mem_buf = ripc_alloc_recv_buf(rcv_size);
                mem_buf.rcv_addr = rcv_addr;
                mem_buf.rcv_size = rcv_size;
                recv_window_list_add(mem_buf);
		return 0;
	}

	mem_buf = used_buf_list_get(rcv_addr); //are we registered yet?
	if (mem_buf.size != -1) {
		DEBUG("Found buffer mr: Rcv_Addr address %p, size %zu",
                      (void *) mem_buf.addr, mem_buf.size);
                mem_buf.rcv_addr = rcv_addr;
                mem_buf.rcv_size = rcv_size;
                used_buf_list_add(mem_buf);
		if ((uint64_t)rcv_addr + rcv_size <= mem_buf.addr + mem_buf.size) { //is the buffer big enough?
                        recv_window_list_add(mem_buf);
			return 0;
		}
		DEBUG("Receive buffer is too small, aborting");
		return 1; //buffer too small, and overlapping buffers are not supported
	}

	// not registered yet
	DEBUG("mr not found, registering memory area");
	if (ripc_buf_register(rcv_addr, rcv_size))
		return 1; //registration failed
	DEBUG("Successfully registered memory area");
	mem_buf = used_buf_list_get(rcv_addr);
        mem_buf.rcv_addr = rcv_addr;
        mem_buf.rcv_size = rcv_size;
        used_buf_list_add(mem_buf);
        recv_window_list_add(mem_buf);
	return 0;
}

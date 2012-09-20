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
	DEBUG("Adding receive window at address %p, size %zu to global pool", mem_buf.rcv_addr, mem_buf.rcv_size);

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

void private_recv_window_list_add(uint16_t service, mem_buf_t mem_buf) {
	DEBUG("Adding receive window at address %p, size %zu to local pool of service %u", mem_buf.rcv_addr, mem_buf.rcv_size, service);

	struct mem_buf_list *container = malloc(sizeof(struct mem_buf_list));
	assert(container);
	memset(container, 0, sizeof(struct mem_buf_list));

	container->next = NULL;
 	container->buf = mem_buf;

	pthread_mutex_lock(&services_mutex);

	struct service_id *service_handle = context.services[service];

	if (service_handle->recv_windows_tail == NULL) {
		service_handle->recv_windows = container;
		service_handle->recv_windows_tail = container;
	} else {
		service_handle->recv_windows_tail->next = container;
		service_handle->recv_windows_tail = container;
	}

	pthread_mutex_unlock(&services_mutex);
	return;
}

void *private_recv_window_list_get(uint16_t service, size_t size) {
	pthread_mutex_lock(&services_mutex);

	struct service_id *service_handle = context.services[service];

	struct mem_buf_list *ptr = service_handle->recv_windows;
	struct mem_buf_list *prev = NULL;

	while(ptr) {
		size_t ptr_size = ptr->buf.rcv_size;
		DEBUG("Checking window list entry: %p size: %zd, requested size: %zd",
				ptr->buf.rcv_addr,
				ptr_size,
				size);
		if (ptr_size >= size) {
			if (prev)
				prev->next = ptr->next;
			else //first element in list
				service_handle->recv_windows = ptr->next;
				//NOTE: If ptr is the only element, then ptr->next is NULL
			if (ptr == service_handle->recv_windows_tail) {
				service_handle->recv_windows_tail = prev;
				if (prev)
					prev->next = NULL;
			}
			pthread_mutex_unlock(&services_mutex);
			void *ret = ptr->buf.rcv_addr;
			free(ptr);
			return ret;
		}
		prev = ptr;
		ptr = ptr->next;
	}
	pthread_mutex_unlock(&services_mutex);
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

void *ripc_buf_realloc(void *buf, size_t size) {
	mem_buf_t oldbuf = used_buf_list_get(buf);
	DEBUG("Resizing buffer %p from %lu to %lu", buf, oldbuf.size, size);
	mem_buf_t newbuf = ripc_resize_recv_buf(
			oldbuf, //NOTE: The buffer is intentionally not re-inserted into the used list!
			size);
	DEBUG("Buffer now at 0x%lu, size %lu", newbuf.addr, newbuf.size);
	return (void *)newbuf.addr;
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

mem_buf_t make_recv_window(void *addr, size_t size) {
	mem_buf_t mem_buf;

	retry:
	DEBUG("Attempting receive window registration at %p, size %u", addr, size);

	if (addr == NULL) { //the user wants us to specify the buffer
		DEBUG("No address specified, allocating new buffer");
		mem_buf = ripc_alloc_recv_buf(size);
		mem_buf.rcv_addr = addr;
		mem_buf.rcv_size = size;
		return mem_buf;
	}

	mem_buf = used_buf_list_get(addr); //are we registered yet?
	if (mem_buf.size != -1) {
		DEBUG("Found buffer mr: Rcv_Addr address %p, size %zu, lkey %#x",
				(void *) mem_buf.addr, mem_buf.size, mem_buf.na->lkey);
		mem_buf.rcv_addr = addr;
		mem_buf.rcv_size = size;
		used_buf_list_add(mem_buf);
		if ((uint64_t)addr + size <= mem_buf.addr + mem_buf.size) { //is the buffer big enough?
			return mem_buf;
		}
		DEBUG("Receive buffer is too small, unregistering (size: %u, requested: %u)", mem_buf.size, size);
		ripc_buf_unregister(addr);
		//fixme: Ignore error as the buffer will be removed from the used list even if deregistration fails
		goto retry;
	}

	// not registered yet
	DEBUG("mr not found, registering memory area (address %p)", addr);
	if (ripc_buf_register(addr, size)) {
		ERROR("memory registration failed (address %p)", addr);
		return invalid_mem_buf; //registration failed
	}
	DEBUG("Successfully registered memory area");
	mem_buf = used_buf_list_get(addr);
	mem_buf.rcv_addr = addr;
	mem_buf.rcv_size = size;
#ifdef HAVE_DEBUG
	ERROR("Got mem_buf:");
	dump_mem_buf(&mem_buf);
#endif
	used_buf_list_add(mem_buf);
	return mem_buf;
}

uint8_t ripc_reg_recv_window(void *rcv_addr, size_t rcv_size) {
	mem_buf_t mem_buf;
	assert(rcv_size > 0);

	mem_buf = make_recv_window(rcv_addr, rcv_size);
	if (mem_buf.size == -1) {
		ERROR("Failed to make receive window at address %p, size %u", rcv_addr, rcv_size);
		return 1; //FAIL
	}

	recv_window_list_add(mem_buf);
	DEBUG("Added receive window to global pool (address: %p, size: %u)", mem_buf.rcv_addr, mem_buf.rcv_size);
	return 0;
}

uint8_t ripc_reg_recv_window_for_service(void *rcv_addr, size_t rcv_size, uint16_t service) {
	mem_buf_t mem_buf;
	assert(rcv_size > 0);

	mem_buf = make_recv_window(rcv_addr, rcv_size);
	if (mem_buf.size == -1) {
		ERROR("Failed to make receive window at address %p, size %u", rcv_addr, rcv_size);
		return 1; //FAIL
	}

	private_recv_window_list_add(service, mem_buf);
	DEBUG("Added receive window to private pool of service %u (address: %p, size: %u)", service, mem_buf.rcv_addr, mem_buf.rcv_size);
	return 0;
}

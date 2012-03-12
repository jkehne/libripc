/*
 * memory.c
 *
 *  Created on: 12.12.2011
 *      Author: jens
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

void used_buf_list_add(struct ibv_mr *item) {
	struct mem_buf_list *container = malloc(sizeof(struct mem_buf_list));
	assert(container);
	memset(container, 0, sizeof(struct mem_buf_list));

	container->mr = item;

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

struct ibv_mr *used_buf_list_get(void *addr) {
	pthread_mutex_lock(&used_list_mutex);

	struct mem_buf_list *ptr = used_buffers;
	struct mem_buf_list *prev = NULL;

	while(ptr) {
		if (((uint64_t)addr >= (uint64_t)ptr->mr->addr)
				&& ((uint64_t)addr < (uint64_t)ptr->mr->addr + ptr->mr->length)) {
			if (prev)
				prev->next = ptr->next;
			else //first element in list
				used_buffers = ptr->next;
				//NOTE: If ptr is the only element, then ptr->next is NULL
			pthread_mutex_unlock(&used_list_mutex);
			struct ibv_mr *ret = ptr->mr;
			free(ptr);
			return ret;
		}
		prev = ptr;
		ptr = ptr->next;
	}
	pthread_mutex_unlock(&used_list_mutex);
	return NULL; //not found
}

void free_buf_list_add(struct ibv_mr *item) {
	struct mem_buf_list *container = malloc(sizeof(struct mem_buf_list));
	assert(container);
	memset(container, 0, sizeof(struct mem_buf_list));

	container->mr = item;
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

struct ibv_mr *free_buf_list_get(size_t size) {
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
		if ((ptr->mr->length >= size) &&
				(ptr->mr->length <= size * 2)) {
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
			struct ibv_mr *ret = ptr->mr;
			free(ptr);
			return ret;
		}
		prev = ptr;
		ptr = ptr->next;
	}
	pthread_mutex_unlock(&free_list_mutex);
	return NULL; //not found
}

void recv_window_list_add(struct ibv_mr *item, void *base, size_t size) {
	DEBUG("Registering receive window at address %p, size %zu",
			base,
			size);

	struct mem_buf_list *container = malloc(sizeof(struct mem_buf_list));
	assert(container);
	memset(container, 0, sizeof(struct mem_buf_list));

	container->mr = item;
	container->next = NULL;
	container->size = size;
	container->base = base;

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
		size_t ptr_size = ptr->size;
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
			void *ret = ptr->base;
			free(ptr);
			return ret;
		}
		prev = ptr;
		ptr = ptr->next;
	}
	pthread_mutex_unlock(&recv_window_mutex);
	return NULL; //not found
}

void return_buf_list_add(uint16_t remote, struct ibv_mr *item) {
	struct mem_buf_list *container = malloc(sizeof(struct mem_buf_list));
	assert(container);
	memset(container, 0, sizeof(struct mem_buf_list));

	container->mr = item;

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

struct ibv_mr *return_buf_list_get(uint16_t remote, size_t size) {
	pthread_mutex_lock(&remotes_mutex);

	struct mem_buf_list *ptr = context.remotes[remote]->return_bufs;
	struct mem_buf_list *prev = NULL;

	while(ptr) {
		if (ptr->mr->length >= size) {
			if (prev)
				prev->next = ptr->next;
			else //first element in list
				context.remotes[remote]->return_bufs = ptr->next;
				//NOTE: If ptr is the only element, then ptr->next is NULL
			pthread_mutex_unlock(&remotes_mutex);
			struct ibv_mr *ret = ptr->mr;
			free(ptr);
			return ret;
		}
		prev = ptr;
		ptr = ptr->next;
	}
	pthread_mutex_unlock(&remotes_mutex);
	return NULL; //not found
}

struct ibv_mr *ripc_alloc_recv_buf(size_t size) {
	if (!size)
		return NULL;

	struct ibv_mr *mr;

	mr = free_buf_list_get(size);
	if (mr) {
		DEBUG("Got hit in free list: Buffer at %p, size %zu", mr->addr, mr->length);
		used_buf_list_add(mr);
		memset(mr->addr, 0, mr->length);
		return mr;
	}

	//none found in cache, so create a new one
	DEBUG("No hit in free list, allocating new mr");
	//mmap correctly aligns and zeroes the buffer.
	void *buf = mmap(
			0,
			size,
			PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_ANONYMOUS,
			-1,
			0);

	if (buf == (void *) -1) {
		ERROR("buffer allocation failed: %s", strerror(errno));
		return NULL;
	}

	assert(buf);
	mr = ibv_reg_mr(
			context.na.pd,
			buf,
			size,
			IBV_ACCESS_LOCAL_WRITE |
			IBV_ACCESS_REMOTE_READ |
			IBV_ACCESS_REMOTE_WRITE);
	DEBUG("mr buffer address is %p, size %zu", mr->addr, mr->length);
	used_buf_list_add(mr);
	return mr;
}

void *ripc_buf_alloc(size_t size) {
	struct ibv_mr *mr = ripc_alloc_recv_buf(size);
	return mr ? mr->addr : NULL;
}

void ripc_buf_free(void *buf) {
	DEBUG("Putting buffer %p into free list", buf);
	struct ibv_mr *mr = used_buf_list_get(buf);
	if (mr)
		free_buf_list_add(mr);
	else {
		DEBUG("Buffer not found!");
	}
}

uint8_t ripc_buf_register(void *buf, size_t size) {
	struct ibv_mr *mr;
	mr = used_buf_list_get(buf);
	if (mr && ((uint64_t)buf + size > (uint64_t)mr->addr + mr->length)) {
		used_buf_list_add(mr);
		return 1; //overlapping buffers are unsupported
	}
	if (!mr)
		mr = ibv_reg_mr(
				context.na.pd,
				buf,
				size,
				IBV_ACCESS_LOCAL_WRITE |
				IBV_ACCESS_REMOTE_READ |
				IBV_ACCESS_REMOTE_WRITE);
	used_buf_list_add(mr);
	return mr ? 0 : 1;
}

void post_new_recv_buf(struct ibv_qp *qp) {
	struct ibv_mr *mr;
	struct ibv_sge *list;
	struct ibv_recv_wr *wr, *bad_wr;
	uint32_t i;

	mr = ripc_alloc_recv_buf(RECV_BUF_SIZE);

	list = malloc(sizeof(struct ibv_sge));
	list->addr = (uint64_t)mr->addr;
	list->length = mr->length;
	list->lkey = mr->lkey;

	wr = malloc(sizeof(struct ibv_recv_wr));
	wr->wr_id = (uint64_t)wr;
	wr->sg_list = list;
	wr->num_sge = 1;
	wr->next = NULL;

	if (ibv_post_recv(qp, wr, &bad_wr)) {
		ERROR("Failed to post receive item to QP %u!", qp->qp_num);
	} else {
		DEBUG("Posted receive buffer at address %p to QP %u",
				mr->addr,
				qp->qp_num);
	}
}

uint8_t ripc_reg_recv_window(void *base, size_t size) {
	struct ibv_mr *mr;
	assert(size > 0);

	if (base == NULL) { //the user wants us to specify the buffer
		DEBUG("No base specified, allocating new buffer");
		mr = ripc_alloc_recv_buf(size);
		recv_window_list_add(mr, base, size);
		return 0;
	}

	mr = used_buf_list_get(base); //are we registered yet?
	if (mr) {
		DEBUG("Found buffer mr: Base address %p, size %zu",
				mr->addr,
				mr->length);
		used_buf_list_add(mr);
		if ((uint64_t)base + size <= (uint64_t)mr->addr + mr->length) { //is the buffer big enough?
			recv_window_list_add(mr, base, size);
			return 0;
		}
		DEBUG("Receive buffer is too small, aborting");
		return 1; //buffer too small, and overlapping buffers are not supported
	}

	// not registered yet
	DEBUG("mr not found, registering memory area");
	if (ripc_buf_register(base, size))
		return 1; //registration failed
	DEBUG("Successfully registered memory area");
	mr = used_buf_list_get(base);
	used_buf_list_add(mr);
	recv_window_list_add(mr, base, size);
	return 0;
}

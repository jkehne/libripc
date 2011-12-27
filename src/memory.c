/*
 * memory.c
 *
 *  Created on: 12.12.2011
 *      Author: jens
 */

#include "ripc.h"
#include "common.h"
#include "memory.h"

struct mem_buf_list {
	struct ibv_mr *mr;
	struct mem_buf_list *next;
};

struct mem_buf_list *used_buffers = NULL;
struct mem_buf_list *available_buffers = NULL;
struct mem_buf_list *available_buffers_tail = NULL;

pthread_mutex_t used_list_mutex, free_list_mutex;

void used_buf_list_add(struct ibv_mr *item) {
	struct mem_buf_list *container = malloc(sizeof(struct mem_buf_list));
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
	struct mem_buf_list *ptr = used_buffers;
	struct mem_buf_list *prev = NULL;

	pthread_mutex_lock(&used_list_mutex);
	while(ptr) {
		if (((uint64_t)addr >= (uint64_t)ptr->mr->addr)
				&& ((uint64_t)addr <= (uint64_t)ptr->mr->addr + ptr->mr->length)) {
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
	struct mem_buf_list *ptr = available_buffers;
	struct mem_buf_list *prev = NULL;

	pthread_mutex_lock(&free_list_mutex);
	while(ptr) {
		if (ptr->mr->length >= size) {
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

struct ibv_mr *ripc_alloc_recv_buf(size_t size) {
	struct ibv_mr *mr;

	mr = free_buf_list_get(size);
	if (mr) {
		DEBUG("Got hit in free list: Buffer at %p, size %u", mr->addr, mr->length);
		used_buf_list_add(mr);
		return mr;
	}

	//none found in cache, so create a new one
	DEBUG("No hit in free list, allocating new mr");
	void *buf = valloc(size);
	//valloc is like malloc, but aligned to page boundary

	if (buf) {
		memset(buf, 0, size);
		mr = ibv_reg_mr(
				context.pd,
				buf,
				size,
				IBV_ACCESS_LOCAL_WRITE |
				IBV_ACCESS_REMOTE_READ |
				IBV_ACCESS_REMOTE_WRITE);
		DEBUG("mr buffer address is %p", mr->addr);
		used_buf_list_add(mr);
		return mr;
	}

	return NULL; //allocation failed
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

struct ibv_mr *ripc_buf_register(void *buf, uint32_t size) {
	struct ibv_mr *mr = ibv_reg_mr(
			context.pd,
			buf,
			size,
			IBV_ACCESS_LOCAL_WRITE |
			IBV_ACCESS_REMOTE_READ |
			IBV_ACCESS_REMOTE_WRITE);
	used_buf_list_add(mr);
	return mr;
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

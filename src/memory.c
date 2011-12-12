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
struct mem_buf_list *avaliable_buffers = NULL;

void used_buf_list_add(struct ibv_mr *item) {
	struct mem_buf_list *container = malloc(sizeof(struct mem_buf_list));
	container->mr = item;
	if (used_buffers == NULL) {
		container->next = NULL;
		used_buffers = container;
	} else {
		container->next = used_buffers;
		used_buffers = container;
	}
	return;
}

struct ibv_mr *used_buf_list_get(void *addr) {
	struct mem_buf_list *ptr = used_buffers;
	struct mem_buf_list *prev = NULL;

	while(ptr) {
		if (ptr->mr->addr == addr) {
			if (prev)
				prev->next = ptr->next;
			else //first element in list
				used_buffers = ptr->next;
				//NOTE: If ptr is the only element, then ptr->next is NULL
			struct ibv_mr *ret = ptr->mr;
			free(ptr);
			return ret;
		}
		prev = ptr;
		ptr = ptr->next;
	}
	return NULL; //not found
}

void free_buf_list_add(struct ibv_mr *item) {
	struct mem_buf_list *container = malloc(sizeof(struct mem_buf_list));
	container->mr = item;
	if (avaliable_buffers == NULL) {
		container->next = NULL;
		avaliable_buffers = container;
	} else {
		container->next = avaliable_buffers;
		avaliable_buffers = container;
	}
	return;
}

struct ibv_mr *free_buf_list_get(void *addr) {
	struct mem_buf_list *ptr = avaliable_buffers;
	struct mem_buf_list *prev = NULL;

	while(ptr) {
		if (ptr->mr->addr == addr) {
			if (prev)
				prev->next = ptr->next;
			else //first element in list
				avaliable_buffers = ptr->next;
				//NOTE: If ptr is the only element, then ptr->next is NULL
			struct ibv_mr *ret = ptr->mr;
			free(ptr);
			return ret;
		}
		prev = ptr;
		ptr = ptr->next;
	}
	return NULL; //not found
}

struct ibv_mr *ripc_alloc_recv_buf(size_t size) {
	//todo: Replace with a slab allocator

	void *buf = valloc(size);
	//valloc is like malloc, but aligned to page boundary
	//+40 for grh

	if (buf != NULL) {
		memset(buf, 0, size);
		struct ibv_mr *mr = ibv_reg_mr(
				context.pd,
				buf,
				size,
				IBV_ACCESS_LOCAL_WRITE);
		used_buf_list_add(mr);
		return mr;
	}

	return NULL;
}

void *ripc_buf_alloc(size_t size) {
	void *buf = valloc(size);
	//valloc is like malloc, but aligned to page boundary
	//+40 for grh

	if (buf != NULL) {
		memset(buf, 0, size);
		struct ibv_mr *mr = ibv_reg_mr(
				context.pd,
				buf,
				size,
				IBV_ACCESS_LOCAL_WRITE |
				IBV_ACCESS_REMOTE_READ |
				IBV_ACCESS_REMOTE_WRITE);
		used_buf_list_add(mr);
		return mr->addr;
	}

	return NULL;
}

void ripc_buf_free(void *buf) {
	struct ibv_mr *mr = used_buf_list_get(buf);
	free_buf_list_add(mr);
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

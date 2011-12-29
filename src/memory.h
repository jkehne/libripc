/*
 * memory.h
 *
 *  Created on: 12.12.2011
 *      Author: jens
 */

#ifndef MEMORY_H_
#define MEMORY_H_

#include <sys/types.h>
#include <pthread.h>

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

void post_new_recv_buf(struct ibv_qp *qp);

struct ibv_mr *ripc_alloc_recv_buf(size_t size);

#endif /* MEMORY_H_ */

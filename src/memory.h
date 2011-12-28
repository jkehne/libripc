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

extern pthread_mutex_t used_list_mutex, free_list_mutex, recv_window_mutex;

struct ibv_mr *used_buf_list_get(void *addr);
void used_buf_list_add(struct ibv_mr *item);
void free_buf_list_add(struct ibv_mr *item);
struct ibv_mr *free_buf_list_get(size_t size);
void post_new_recv_buf(struct ibv_qp *qp);

struct ibv_mr *ripc_alloc_recv_buf(size_t size);

#endif /* MEMORY_H_ */

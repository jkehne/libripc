/*
 * memory.h
 *
 *  Created on: 12.12.2011
 *      Author: jens
 */

#ifndef MEMORY_H_
#define MEMORY_H_

#include <sys/types.h>

struct ibv_mr *used_buf_list_get(void *addr);
void used_buf_list_add(struct ibv_mr *item);
void free_buf_list_add(struct ibv_mr *item);
struct ibv_mr *free_buf_list_get(void *addr);

struct ibv_mr *ripc_alloc_recv_buf(size_t size);
struct ibv_mr *ripc_buf_register(void *buf, uint32_t size);

#endif /* MEMORY_H_ */

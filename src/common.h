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
#ifndef COMMON_H_
#define COMMON_H_

#ifdef HAVE_STDBOOL_H
# include <stdbool.h>
#else
# ifndef HAVE__BOOL
#  ifdef __cplusplus
typedef bool _Bool;
#  else
#   define _Bool signed char
#  endif
# endif
# define bool _Bool
# define false 0
# define true 1
# define __bool_true_false_are_defined 1
#endif

#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/syscall.h>
#ifdef NETARCH_INFINIBAND
#include <infiniband/common.h>
#endif
#ifdef NETARCH_BGP
#include <bgp/common.h>
#endif
#ifdef NETARCH_LOCAL
#include <local/common.h>
#endif

#include <memory.h>

#define ERROR(...) do { fprintf(stderr, "Thread %u: %s() (%s, line %u): ", (uint32_t)syscall(SYS_gettid), __PRETTY_FUNCTION__, __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr,"\n"); } while (0)
#define panic(...) do { fprintf(stderr, "Thread %u: %s() (%s, line %u): FATAL: ", (int) pthread_self(), __PRETTY_FUNCTION__, __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr,"\n"); exit(EXIT_FAILURE); } while (0)

#ifdef HAVE_DEBUG
#define DEBUG(...) ERROR(__VA_ARGS__)
#else
#define DEBUG(...) do { } while (0)
#endif


enum msg_type {
       RIPC_MSG_SEND = 0xdeadbeef,
       RIPC_MSG_INTERRUPT,
       RIPC_MSG_RESOLVE_REQ,
       RIPC_MSG_RESOLVE_REPLY,
       RIPC_RDMA_CONN_REQ,
       RIPC_RDMA_CONN_REPLY
};

enum conn_state {
       RIPC_RDMA_DISCONNECTED,
       RIPC_RDMA_CONNECTING,
       RIPC_RDMA_ESTABLISHED
};


struct service_id {
	uint16_t number;
	bool is_multicast;
    struct mem_buf_list *recv_windows;
    struct mem_buf_list *recv_windows_tail;
    struct netarch_service_id na;
};

struct remote_context {
	enum conn_state state;
	struct mem_buf_list *return_bufs;
	struct netarch_remote_context na;
};

struct library_context {
        bool initialized;
	struct netarch_library_context na;
	struct service_id *services[UINT16_MAX];
	struct remote_context *remotes[UINT16_MAX];
};

struct msg_header {
	enum msg_type type;
	uint16_t from;
	uint16_t to;
	uint16_t short_words;
	uint16_t long_words;
	uint16_t new_return_bufs;
};

struct short_header {
	uint32_t offset;
	uint32_t size;
};

struct long_desc {
	uint64_t addr;
	size_t length;
	uint32_t rkey;
	uint8_t transferred;
};

extern struct library_context context;
extern pthread_mutex_t services_mutex, remotes_mutex;

#endif /* COMMON_H_ */

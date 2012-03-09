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
#ifdef NETARCH_INFINIBAND
#include <infiniband/common.h>
#endif
#ifdef NETARCH_BGP
#include <bgp/common.h>
#endif
#ifdef NETARCH_LOCAL
#include <local/common.h>
#endif

#include "memory.h"

#define ERROR(...) fprintf(stderr, "Thread %d: %s() (%s, line %u): ", (int) pthread_self(), __PRETTY_FUNCTION__, __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr,"\n")
#define panic(...) fprintf(stderr, "Thread %d: %s() (%s, line %u): FATAL: ", (int) pthread_self(), __PRETTY_FUNCTION__, __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr,"\n"); exit(EXIT_FAILURE)

#ifdef HAVE_DEBUG
#define DEBUG(...) ERROR(__VA_ARGS__)
#else
#define DEBUG(...)
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
        struct netarch_service_id na;
};

struct remote_context {
	enum conn_state state;
	struct mem_buf_list *return_bufs;
	uint32_t resolver_qp;
	uint32_t qp_num;
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

#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <memory.h> // Mem bufs.
#include "netarch_context.h"

/**
 * Context required by libRIPC to SEND TO that destination.
 */
struct context_sending {
	bool is_multicast;
	enum conn_state state;
	struct mem_buf_list *return_bufs;
	struct netarch_sending na;
};

/**
 * Context required by libRIPC to RECEIVE AS a certain process.
 */
struct context_receiving {
	struct mem_buf_list *recv_windows;
	struct mem_buf_list *recv_windows_tail;
	struct netarch_receiving na;
};

#endif // CONTEXT_H_

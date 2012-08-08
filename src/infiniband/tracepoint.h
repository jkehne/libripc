/*  Copyright 2011, 2012 Jens Kehne
 *  Copyright 2012 Jan Stoess, Karlsruhe Institute of Technology
 *  Copyright 2012 Marius Hillenbrand, Karlsruhe Institute of Technology
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

#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER libripc_ibv

#undef TRACEPOINT_INCLUDE_FILE
#define TRACEPOINT_INCLUDE_FILE ./tracepoint.h 

#ifdef __cplusplus
#extern "C"{
#endif

#if !defined (_LIBRIPC_IBVERBS_PROVIDER_H) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define _LIBRIPC_IBVERBS_PROVIDER_H

#include <lttng/tracepoint.h>

// TODO define tracepoints

TRACEPOINT_EVENT(
		libripc_ibv, /* provider name */
		reg_mr, /* tracepoint name */
		TP_ARGS(unsigned long, pd, unsigned long, addr,
			unsigned long, length, int, access,
			unsigned long, mr),
		TP_FIELDS(
			ctf_integer_hex(unsigned long, pd, pd)
			ctf_integer_hex(unsigned long, addr, addr)
			ctf_integer(unsigned long, length, length)
			ctf_integer_hex(int, access, access)
			ctf_integer_hex(unsigned long, mr, mr)
		)
)

TRACEPOINT_EVENT(
		libripc_ibv,
		dereg_mr,
		TP_ARGS(unsigned long, mr, int, status),
		TP_FIELDS(
			ctf_integer_hex(unsigned long, mr, mr)
			ctf_integer(int, status, status)
		)
)

TRACEPOINT_EVENT(
		libripc_ibv,
		create_comp_channel,
		TP_ARGS(unsigned long, context,
			unsigned long, comp_channel),
		TP_FIELDS(
			ctf_integer_hex(unsigned long, context, context)
			ctf_integer_hex(unsigned long, comp_channel, comp_channel)
		)
)

TRACEPOINT_EVENT(
		libripc_ibv,
		destroy_comp_channel,
		TP_ARGS(unsigned long, comp_channel, int, status),
		TP_FIELDS(
			ctf_integer_hex(unsigned long, comp_channel, comp_channel)
			ctf_integer(int, status, status)
		)
)

TRACEPOINT_EVENT(
		libripc_ibv,
		create_cq,
		TP_ARGS(unsigned long, context, int, cqe,
			unsigned long, cq_context,
			unsigned long, comp_channel,
			int, comp_vector,
			unsigned long, cq),
		TP_FIELDS(
			ctf_integer_hex(unsigned long, context, context)
			ctf_integer(int, cqe, cqe)
			ctf_integer_hex(unsigned long, cq_context, cq_context)
			ctf_integer_hex(unsigned long, comp_channel, comp_channel)
			ctf_integer_hex(unsigned long, comp_channel, comp_channel)
			ctf_integer(int, cq, cq)
		)
)

TRACEPOINT_EVENT(
		libripc_ibv,
		destroy_cq,
		TP_ARGS(unsigned long, cq, int, status),
		TP_FIELDS(
			ctf_integer_hex(unsigned long, cq, cq),
			ctf_integer(int, status, status)
		)
)

TRACEPOINT_EVENT(
		libripc_ibv,
		poll_cq,
		TP_ARGS(unsigned long, cq, int, num_entries, int, result),
		TP_FIELDS(
			ctf_integer_hex(unsigned long, cq, cq)
			ctf_integer(int, num_entries, num_entries)
			ctf_integer(int, result, result)
		)
)

TRACEPOINT_EVENT(
		libripc_ibv,
		post_send,
		TP_ARGS(unsigned long, qp, unsigned long, wr, int, status),
		TP_FIELDS(
			ctf_integer_hex(unsigned long, qp, qp)
			ctf_integer_hex(unsigned long, wr, wr)
			ctf_integer(int, status, status)
		)
)
	
TRACEPOINT_EVENT(
		libripc_ibv,
		post_recv,
		TP_ARGS(unsigned long, qp, unsigned long, wr, int, status),
		TP_FIELDS(
			ctf_integer_hex(unsigned long, qp, qp)
			ctf_integer_hex(unsigned long, wr, wr)
			ctf_integer(int, status, status)
		)
)
		

#endif /* _LIBRIPC_IBVERBS_PROVIDER_H */

#include <lttng/tracepoint-event.h>

#ifdef __cplusplus
}
#endif /* __cplusplus */


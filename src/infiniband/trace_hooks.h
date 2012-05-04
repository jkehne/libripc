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
#ifndef TRACE_HOOKS_H_
#define TRACE_HOOKS_H_

#define ENABLE_IBV_TRACING

#ifdef ENABLE_IBV_TRACING

#include <infiniband/verbs.h>
// functions that actually call tracepoints and verbs functions
/*
struct ibv_device **hooked_ibv_get_device_list(int *num_devices);

void hooked_ibv_free_device_list(struct ibv_device **list);

const char *hooked_ibv_get_device_name(struct ibv_device *device);

uint64_t hooked_ibv_get_device_guid(struct ibv_device *device);
*/
struct ibv_context *hooked_ibv_open_device(struct ibv_device *device);

int hooked_ibv_close_device(struct ibv_context *context);
/*
int hooked_ibv_get_async_event(struct ibv_context *context,
			struct ibv_async_event *event);

void hooked_ibv_ack_async_event(struct ibv_async_event *event);

int hooked_ibv_query_device(struct ibv_context *context,
		     struct ibv_device_attr *device_attr);

int hooked_ibv_query_port(struct ibv_context *context, uint8_t port_num,
		   struct ibv_port_attr *port_attr);

int hooked_ibv_query_gid(struct ibv_context *context, uint8_t port_num,
		  int index, union hooked_ibv_gid *gid);

int hooked_ibv_query_pkey(struct ibv_context *context, uint8_t port_num,
		   int index, uint16_t *pkey);
*/
struct ibv_pd *hooked_ibv_alloc_pd(struct ibv_context *context);

int hooked_ibv_dealloc_pd(struct ibv_pd *pd);

struct ibv_mr *hooked_ibv_reg_mr(struct ibv_pd *pd, void *addr,
			  size_t length, int access);

int hooked_ibv_dereg_mr(struct ibv_mr *mr);

struct ibv_comp_channel *hooked_ibv_create_comp_channel(struct ibv_context *context);

int hooked_ibv_destroy_comp_channel(struct ibv_comp_channel *channel);

struct ibv_cq *hooked_ibv_create_cq(struct ibv_context *context, int cqe,
			     void *cq_context,
			     struct ibv_comp_channel *channel,
			     int comp_vector);

int hooked_ibv_resize_cq(struct ibv_cq *cq, int cqe);

int hooked_ibv_destroy_cq(struct ibv_cq *cq);

int hooked_ibv_get_cq_event(struct ibv_comp_channel *channel,
		     struct ibv_cq **cq, void **cq_context);

void hooked_ibv_ack_cq_events(struct ibv_cq *cq, unsigned int nevents);

static inline int hooked_ibv_poll_cq(struct ibv_cq *cq, int num_entries, struct ibv_wc *wc)
{
	return cq->context->ops.poll_cq(cq, num_entries, wc);
}

static inline int hooked_ibv_req_notify_cq(struct ibv_cq *cq, int solicited_only)
{
	return cq->context->ops.req_notify_cq(cq, solicited_only);
}
/*
struct ibv_srq *hooked_ibv_create_srq(struct ibv_pd *pd,
			       struct ibv_srq_init_attr *srq_init_attr);

int hooked_ibv_modify_srq(struct ibv_srq *srq,
		   struct ibv_srq_attr *srq_attr,
		   int srq_attr_mask);

int hooked_ibv_query_srq(struct ibv_srq *srq, struct ibv_srq_attr *srq_attr);

int hooked_ibv_destroy_srq(struct ibv_srq *srq);

static inline int hooked_ibv_post_srq_recv(struct ibv_srq *srq,
				    struct ibv_recv_wr *recv_wr,
				    struct ibv_recv_wr **bad_recv_wr)
{
	return srq->context->ops.post_srq_recv(srq, recv_wr, bad_recv_wr);
}
*/
struct ibv_qp *hooked_ibv_create_qp(struct ibv_pd *pd,
			     struct ibv_qp_init_attr *qp_init_attr);

int hooked_ibv_modify_qp(struct ibv_qp *qp, struct ibv_qp_attr *attr,
		  int attr_mask);

int hooked_ibv_query_qp(struct ibv_qp *qp, struct ibv_qp_attr *attr,
		 int attr_mask,
		 struct ibv_qp_init_attr *init_attr);

int hooked_ibv_destroy_qp(struct ibv_qp *qp);

static inline int hooked_ibv_post_send(struct ibv_qp *qp, struct ibv_send_wr *wr,
				struct ibv_send_wr **bad_wr)
{
	return qp->context->ops.post_send(qp, wr, bad_wr);
}

static inline int hooked_ibv_post_recv(struct ibv_qp *qp, struct ibv_recv_wr *wr,
				struct ibv_recv_wr **bad_wr)
{
	return qp->context->ops.post_recv(qp, wr, bad_wr);
}
/*
struct ibv_ah *hooked_ibv_create_ah(struct ibv_pd *pd, struct ibv_ah_attr *attr);

int hooked_ibv_init_ah_from_wc(struct ibv_context *context, uint8_t port_num,
			struct ibv_wc *wc, struct ibv_grh *grh,
			struct ibv_ah_attr *ah_attr);

struct ibv_ah *hooked_ibv_create_ah_from_wc(struct ibv_pd *pd, struct ibv_wc *wc,
				     struct ibv_grh *grh, uint8_t port_num);

int hooked_ibv_destroy_ah(struct ibv_ah *ah);

int hooked_ibv_attach_mcast(struct ibv_qp *qp, const union hooked_ibv_gid *gid, uint16_t lid);

int hooked_ibv_detach_mcast(struct ibv_qp *qp, const union hooked_ibv_gid *gid, uint16_t lid);

int hooked_ibv_fork_init(void);
*/

// define hooks
/* #define ibv_get_device_list(...) hooked_ibv_get_device_list(__VA_ARGS__) 

#define ibv_free_device_list(...) hooked_ibv_free_device_list(__VA_ARGS__)

#define ibv_get_device_name(...) hooked_ibv_get_device_name(__VA_ARGS__)

#define ibv_get_device_guid(...) hooked_ibv_get_device_guid(__VA_ARGS__)
*/
#define ibv_open_device(...) hooked_ibv_open_device(__VA_ARGS__)

#define ibv_close_device(...) hooked_ibv_close_device(__VA_ARGS__)

/*
#define ibv_get_async_event(...) hooked_ibv_get_async_event(__VA_ARGS__)

#define ibv_ack_async_event(...) hooked_ibv_ack_async_event(__VA_ARGS__)

#define ibv_query_device(...) hooked_ibv_query_device(__VA_ARGS__)

#define ibv_query_port(...) hooked_ibv_query_port(__VA_ARGS__)

#define ibv_query_gid(...) hooked_ibv_query_gid(__VA_ARGS__) 

#define ibv_query_pkey(...) hooked_ibv_query_pkey(__VA_ARGS__)
*/
#define ibv_alloc_pd(...) hooked_ibv_alloc_pd(__VA_ARGS__)

#define ibv_dealloc_pd(...) hooked_ibv_dealloc_pd(__VA_ARGS__)

#define ibv_reg_mr(...) hooked_ibv_reg_mr(__VA_ARGS__)

#define ibv_dereg_mr(...) hooked_ibv_dereg_mr(__VA_ARGS__)

#define ibv_create_comp_channel(...) hooked_ibv_create_comp_channel(__VA_ARGS__)

#define ibv_destroy_comp_channel(...) hooked_ibv_destroy_comp_channel(__VA_ARGS__)

#define ibv_create_cq(...) hooked_ibv_create_cq(__VA_ARGS__)

#define ibv_resize_cq(...) hooked_ibv_resize_cq(__VA_ARGS__)

#define ibv_destroy_cq(...) hooked_ibv_destroy_cq(__VA_ARGS__)

#define ibv_get_cq_event(...) hooked_ibv_get_cq_event(__VA_ARGS__)

#define ibv_ack_cq_events(...) hooked_ibv_ack_cq_events(__VA_ARGS__)

#define ibv_poll_cq(...) hooked_ibv_poll_cq(__VA_ARGS__)

#define ibv_req_notify_cq(...) hooked_ibv_req_notify_cq(__VA_ARGS__)
/*
#define ibv_create_srq(...) hooked_ibv_create_srq(__VA_ARGS__)

#define ibv_modify_srq(...) hooked_ibv_modify_srq(__VA_ARGS__)

#define ibv_query_srq(...) hooked_ibv_query_srq(__VA_ARGS__)

#define ibv_destroy_srq(...) hooked_ibv_destroy_srq(__VA_ARGS__)

#define ibv_post_srq_recv(...) hooked_ibv_post_srq_recv(__VA_ARGS__)
*/
#define ibv_create_qp(...) hooked_ibv_create_qp(__VA_ARGS__)

#define ibv_modify_qp(...) hooked_ibv_modify_qp(__VA_ARGS__)

#define ibv_query_qp(...) hooked_ibv_query_qp(__VA_ARGS__)

#define ibv_destroy_qp(...) hooked_ibv_destroy_qp(__VA_ARGS__)

#define ibv_post_send(...) hooked_ibv_post_send(__VA_ARGS__)

#define ibv_post_recv(...) hooked_ibv_post_recv(__VA_ARGS__)
/*
#define ibv_create_ah(...) hooked_ibv_create_ah(__VA_ARGS__)

#define ibv_init_ah_from_wc(...) hooked_ibv_init_ah_from_wc(__VA_ARGS__)

#define ibv_create_ah_from_wc(...) hooked_ibv_create_ah_from_wc(__VA_ARGS__)

#define ibv_destroy_ah(...) hooked_ibv_destroy_ah(__VA_ARGS__)

#define ibv_attach_mcast(...) hooked_ibv_attach_mcast(__VA_ARGS__)

#define ibv_detach_mcast(...) hooked_ibv_detach_mcast(__VA_ARGS__)

#define ibv_fork_init() hooked_ibv_fork_init()
*/




#else
// ibv_... functions calls plainly call verbs functions


#endif

#endif

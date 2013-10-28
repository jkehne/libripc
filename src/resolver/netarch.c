#include <stdio.h>
#include "netarch.h"

int netarch_store_sendctx_in_cap(struct capability *ptr, struct netarch_address_record *data)
{
	struct ibv_ah_attr ah_attr = { 0 };

	if (!ptr->send) {
		DEBUG("'%p' did not have sendctx yet, allocating...", ptr);
		ptr->send = malloc(sizeof (struct context_sending));
		memset(ptr->send, 0, sizeof (struct context_sending));

		/* I guess we can reuse the old completion queue. */
		/* TODO: The following block is copied from
		 *       alloc_queue_state2(). Make that method reusable. */
		ptr->send->na.cq = ibv_create_cq(
			context.na.device_context,
			100,
			NULL,
			NULL,
			0);
		if (ptr->send->na.cq == NULL) {
			ERROR("Failed to allocate send completion queue!");
			return GENERIC_ERROR;
		} else {
			DEBUG("Allocated send completion queue: %u", (ptr->send->na.cq)->handle);
		}
	}

	ah_attr.dlid = data->lid;
	ah_attr.port_num = context.na.port_num;
	ah_attr.sl = 7;

	ptr->send->na.qp_num = data->qp_num;
	ptr->send->na.ah = ibv_create_ah(context.na.pd, &ah_attr);

	DEBUG("Created sendctx '%p'={ '%d', '%p'={ dlid='%d' } }.", ptr->send, ptr->send->na.qp_num, ptr->send->na.ah, ah_attr.dlid);

	return SUCCESS;
}

int netarch_read_sendctx_from_cap(struct capability *ptr, struct netarch_address_record *data)
{
	data->lid = context.na.lid;
	data->qp_num = ptr->send->na.qp_num;

	DEBUG("Netarch Record '%p' is { '%d', '%d' }", data, data->lid, data->qp_num);

	return SUCCESS;
}

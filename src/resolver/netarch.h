#ifndef NETARCH_H_
#define NETARCH_H_

#include "capability.h"
#include "netarch_context.h"

int netarch_store_sendctx_in_cap(struct capability *ptr, struct netarch_address_record *data);
int netarch_read_sendctx_from_cap(struct capability *ptr, struct netarch_address_record *data);

#endif

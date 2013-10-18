#ifndef ZKADAPTER_H_
#define ZKADAPTER_H_

#include <zookeeper/zookeeper.h>
#include "capability.h"
#include "naming.h"


uint32_t zka_connection_get_id();

int zka_service_create(Capability cap);

void zka_name_servers_setup();

int zka_add_auth(Capability cap);

int zka_lookup_once(Capability cap);
int zka_lookup(Capability cap, void (*callback)(Capability));
int zka_set_address(Capability cap);
int zka_disable_updates(Capability cap);

#endif // ZKADAPTER_H_

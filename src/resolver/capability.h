#ifndef CAPABILITY_H_
#define CAPABILITY_H_

#include <stdint.h>
#include <stddef.h>
#include "naming.h"

#define LEN_SERVICE_PASS 63
#define LEN_SERVICE_ADDR 63

#define MAX_CAPS 256

/**
 * Represents a capability for a ZNode.
 *
 * auth_id Contains the number of the last session in which authentication
 *         has been performed for this capability.
 */
struct capability
{
	char name[LEN_SERVICE_NAME + 1];
	char pass[LEN_SERVICE_PASS + 1];
	char addr[LEN_SERVICE_ADDR + 1];
	unsigned int auth_id;
};

Capability capability_create_empty();

int capability_contains_authinfo(Capability cap, const char *func_name);

int capability_exists(Capability cap, const char *func_name);

Capability capability_of_struct(struct capability *ptr);
struct capability *capability_get(Capability cap);

const char *capability_get_service_pass(Capability cap);
const char *capability_get_service_addr(Capability cap);
int capability_set_address(Capability cap, const char *address);
int capability_set_name(Capability cap, const char *name);
int capability_auth(Capability cap);
int check_service_name(const char *name, const char *func_name);

#endif /* CAPABILITY_H_ */

/**
 * Contains the API of the Naming Service.
 */

#ifndef NAMING_H_
#define NAMING_H_

#include <stddef.h>
#include <stdint.h>

#define LEN_SERVICE_NAME 63
#define INVALID_CAPABILITY SIZE_MAX

enum RESULT
{
	GENERIC_ERROR = -1,
	SUCCESS,
	SERVICE_EXISTS,
	NO_SUCH_SERVICE
};

typedef size_t Capability;

Capability capability_create(const char *servicename);

void capability_free(Capability cap);

int capability_serialize(Capability cap, const char* filename);

Capability capability_deserialize(const char *filename);

const char *capability_get_service_name(Capability cap);

void capability_debug(Capability cap);


/* Configuration: */

void name_servers_set(const char* addresses);

void name_servers_setup();

/* Service Management */

int service_create(Capability cap);

int service_login(Capability cap);

int service_logout(Capability cap);

Capability service_lookup_once(const char *servicename);

Capability service_lookup(const char *servicename, void (*onChange)(Capability));

int service_update(Capability cap, void (*onChange)(Capability));

int service_update_once(Capability cap);

int service_update_disable(Capability cap);

// TODO: void delete_service(Capability cap) is missing
//       -- can only be realized via proxy/gateway/watchdog

#endif /* NAMING_H_ */

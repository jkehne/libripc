#include <zookeeper/zookeeper.h>
#include <string.h>
#include <stdarg.h>

#include "naming.h"
#include "capability.h"
#include "zkadapter.h"
#include "address.h"


Capability service_create_capability(const char *servicename, const char *func_name)
{
	Capability result = capability_create_empty();
	if (result == INVALID_CAPABILITY) {
		fprintf(stderr, "%s(): Could not create capability for service \"%s\".\n",
				func_name, servicename);
		return INVALID_CAPABILITY;
	}

	if (capability_set_name(result, servicename) != SUCCESS) {
		fprintf(stderr, "%s(): Could not set service name \"%s\" for capability \"%d\".\n",
				func_name, servicename, result);
		capability_free(result);
		return INVALID_CAPABILITY;
	}

	return result;
}

Capability service_lookup_once(const char *servicename)
{
	Capability result = service_create_capability(servicename, "service_lookup_once");
	if (result == INVALID_CAPABILITY) {
		return INVALID_CAPABILITY;
	}

	int zresult = zka_lookup_once(result);
	if (zresult == NO_SUCH_SERVICE) {
		fprintf(stderr, "service_lookup_once(): "
				"Service \"%s\" does not exist.\n",
				servicename);
		capability_free(result);
		return INVALID_CAPABILITY;
	} else if (zresult != SUCCESS) {
		fprintf(stderr, "service_lookup_once(): "
				"Generic error while trying to look up service \"%s\".\n",
				servicename);
		capability_free(result);
		return INVALID_CAPABILITY;
	}

	return result;
}

Capability service_lookup(const char *servicename, void (*callback)(Capability))
{
	Capability result = service_create_capability(servicename, "service_lookup");
	if (result == INVALID_CAPABILITY) {
		return INVALID_CAPABILITY;
	}

	int zresult = zka_lookup(result, callback);
	if (zresult == NO_SUCH_SERVICE) {
		fprintf(stderr, "service_lookup(): "
				"Service \"%s\" does not exist.\n",
				servicename);
		capability_free(result);
		return INVALID_CAPABILITY;
	} else if (zresult != SUCCESS) {
		fprintf(stderr, "service_lookup(): "
				"Generic error while trying to look up service \"%s\".\n",
				servicename);
		capability_free(result);
		return INVALID_CAPABILITY;
	}

	return result;
}

int service_create(Capability cap)
{
	// TODO: Do we want to check whether addr != ""?
	//       This is an issue of semantics.
	int result = capability_contains_authinfo(cap, "service_create");
	if (result != SUCCESS) {
		return result;
	}

	result = zka_service_create(cap);
	if (result == SERVICE_EXISTS) {
		fprintf(stderr, "service_create(): "
				"Service \"%s\" does already exist.\n",
				capability_get_service_name(cap));
		return result;
	} else if (result != SUCCESS) {
		fprintf(stderr, "service_create(): "
				"Creation failed.\n");
		return result;
	}

	return SUCCESS;
}

int service_login(Capability cap)
{
	int result = capability_contains_authinfo(cap, "service_login");
	if (result != SUCCESS) {
		return result;
	}

	result = capability_auth(cap);
	if (result != SUCCESS) {
		fprintf(stderr, "service_login(): "
				"Authentication failed.\n");
		return result;
	}

	result = capability_set_recvctx(cap);
	if (result != SUCCESS) {
		fprintf(stderr, "service_login(): "
				"Failed to get current hardware info.\n");
		return result;
	}

	// Included in alloc_q_state which is called by recvctx
	/* result = capability_set_sendctx(cap); */
	/* if (result != SUCCESS) { */
		/* fprintf(stderr, "service_login(): " */
				/* "Got hardware info but failed to prepare addressing info.\n"); */
		/* return result; */
	/* } */

	result = zka_set_address(cap);
	if (result != SUCCESS) {
		fprintf(stderr, "service_login(): "
				"Failed to send data.\n");
		return result;
	}

	return SUCCESS;
}

int service_logout(Capability cap)
{
	/* Are we theoretically able to update ZK? */
	int result = capability_contains_authinfo(cap, "service_logout");
	if (result != SUCCESS) {
		return result;
	}

	/* Are we practically able to update ZK? */
	result = capability_auth(cap);
	if (result != SUCCESS) {
		fprintf(stderr, "service_logout(): "
				"Authentication failed.\n");
		return result;
	}

	/* Announce that we are offline *before* actually going offline. */
	result = zka_set_offline(cap);
	if (result != SUCCESS) {
		fprintf(stderr, "service_logout(): "
				"Failed to log service out in ZK.\n");
	}

	result = capability_clear_sendctx(capability_get(cap)); // TODO error hdnl
	result = capability_clear_recvctx(cap);
	if (result != SUCCESS) {
		fprintf(stderr, "service_logout(): "
				"Could not clear local address.\n");
		return result;
	}

	return result;
}

int service_update(Capability cap, void (*callback)(Capability))
{
	int zresult = zka_lookup(cap, callback);
	if (zresult == NO_SUCH_SERVICE) {
		fprintf(stderr, "service_update(): "
				"Service does not exist.\n");
		return GENERIC_ERROR;
	} else if (zresult != SUCCESS) {
		fprintf(stderr, "service_update(): "
				"Failed to perform service lookup.\n");
		return GENERIC_ERROR;
	}

	return SUCCESS;
}

int service_update_once(Capability cap)
{
	int zresult = zka_lookup_once(cap);
	if (zresult == NO_SUCH_SERVICE) {
		fprintf(stderr, "service_update_once(): "
				"Service does not exist.\n");
		return GENERIC_ERROR;
	} else if (zresult != SUCCESS) {
		fprintf(stderr, "service_update_once(): "
				"Failed to perform service lookup.\n");
		return GENERIC_ERROR;
	}

	return SUCCESS;
}

int service_update_disable(Capability cap)
{
	int result = capability_exists(cap, "service_update_disable");
	if (result != SUCCESS) {
		return result;
	}

	result = zka_disable_updates(cap);
	if (result != SUCCESS) {
		fprintf(stderr, "service_update_disable(): "
				"Could not disable updates for capability.\n");
	}

	return result;
}

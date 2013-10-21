#include <zookeeper/zookeeper.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <stdint.h>
#include <string.h>
#include "zkadapter.h"
#include "capability.h"
#include "base64.h"

// TODO: Refactor functions to expect struct capability *

#define ZK_CHROOT "/libRIPC"
#define ZK_TIMEOUT 10000
#define ZK_LOGLEVEL ZOO_LOG_LEVEL_WARN

/**
 * Maximum string length (excluding trailing \0) for
 * ZooKeeper credentials in the form user:pass.
 *
 * Note that this is used for user:base64(sha1(user:pass))
 * when creating ACLs as well as for "raw" user:pass when
 * adding authentication information to the client's connection.
 *
 * Thus, ensure that it is large enough
 * (LEN_SERVICE_NAME, LEN_SERVICE_PASS, strlen(base64(SHA_DIGEST_LENGTH))).
 */
#define LEN_ZK_CREDS 2047

struct zka_watcher_context
{
	/**
	 * Pointer to the corresponding capability data structure.
	 *
	 * If a capability is freed via capability_free(), this pointer will be
	 * invalid. Check that is_enabled is true before accessing that pointer!
	 *
	 * capability_free() must ensure that it sets is_enabled to false.
	 */
	struct capability *ptr;
	/**
	 * Application-supplied pointer to callback function, to be called after
	 * capability has been updated.
	 */
	void (*callback)(Capability);
	/**
	 * True as long as the client shall react to a watch notification.
	 * If false, the memory occupied by this data structure can be freed.
	 */
	uint8_t is_enabled;
};

// FIXME: If a watch is set multiple times for the same capability,
//        the context objects of all but the last watch will only be free()'d
//        when they are triggered. They are "forgotten" in the array.
struct zka_watcher_context *zka_wctxs[MAX_CAPS] = { 0 };

char *srv = NULL;
zhandle_t *zk = NULL;
const clientid_t *zid = NULL;
uint32_t sid = 0;

/* Mutual dependency with zka_watcher_node(). */
int zka_wget(const char *path, struct capability *ptr, struct zka_watcher_context *wctx);

void zka_watcher_node(zhandle_t *zh, int type, int state, const char *path, void *context)
{
	(void) zh; // We use only one connection, we don't need the handle.

	if (state != ZOO_CONNECTED_STATE) {
		fprintf(stderr, "zka_watcher_node(): "
				"Watcher function called when there was no connection.\n");
		return;
	}

	if (!context) {
		fprintf(stderr, "zka_watcher_node(): "
				"Context is NULL.\n");
		return;
	}

	struct zka_watcher_context *wctx = context;

	if (!wctx->is_enabled) {
#ifdef DEBUG
		fprintf(stderr, "zka_watcher_context(): "
				"Found dead watch for \"%s\". "
				"Freeing and stopping event handling for that context.\n",
				path);
#endif
		/* Might still be in array. Prevent double free / segfault. */
		size_t i;
		for (i = 0; i < MAX_CAPS; ++i) {
			if (zka_wctxs[i] == wctx) {
				zka_wctxs[i] = NULL;
			}
		}

		free(wctx);
		return;
	}

	int requires_callback = 0;
	if (type == ZOO_DELETED_EVENT) {
#ifdef DEBUG
		fprintf(stderr, "zka_watcher_node(): "
				"Service just vanished.\n");
#endif
		memset(wctx->ptr->addr, 0, LEN_SERVICE_ADDR + 1); // Trailing \0.
		requires_callback = 1;
	} else if (type == ZOO_CHANGED_EVENT) {
		int result = zka_wget(path, wctx->ptr, wctx);
		if (result == NO_SUCH_SERVICE) {
#ifdef DEBUG
			fprintf(stderr, "zka_watcher_node(): "
					"Service just vanished.\n");
#endif
			/* Buffer has already been nulled by zka_wget(). */
			requires_callback = 1;
		} else if (result != SUCCESS) {
			fprintf(stderr, "zka_watcher_node(): "
					"Failed to get changes from ZK.\n");
		} else { // SUCCESS
			requires_callback = 1;
		}
	} else {
		fprintf(stderr, "zka_watcher_node(): "
				"Ook?\n");
	}

	if (requires_callback) {
		wctx->callback(capability_of_struct(wctx->ptr));
	}
}

void zka_clean_addr(struct capability *ptr)
{
	/* The zoo_*get() family of functions might be called as part of an
	 * update to an already valid capability, containing an address string.
	 *
	 * At least the ZK CLI does not send null-terminated strings to ZK.
	 * ZK itself does not clean the buffer we supply, so we will read
	 * garbage if strlen(new) < strlen(old).
	 */
	memset(ptr->addr, 0, LEN_SERVICE_ADDR + 1); // Trailing \0.
}

int zka_wget(const char *path, struct capability *ptr, struct zka_watcher_context *wctx)
{
	zka_clean_addr(ptr);

	int zlength = LEN_SERVICE_ADDR;
	int zresult = zoo_wget(zk, path, zka_watcher_node, wctx, ptr->addr, &zlength, NULL);
	if (zresult != ZOK) {
		if (zresult == ZNONODE) {
#ifdef DEBUG
			fprintf(stderr, "zoo_wget(): %s\n", zerror(zresult));
			fprintf(stderr, "zka_wget(): "
					"Service does not exist.\n");
#endif
			return NO_SUCH_SERVICE;
		} else {
			fprintf(stderr, "zoo_wget(): %s\n", zerror(zresult));
			fprintf(stderr, "zka_wget(): "
					"Failed to get data from ZK.\n");
			return GENERIC_ERROR;
		}
	}

	return SUCCESS;
}

/**
 * Encodes user name and passphrase into a credential string as used by 
 * ZooKeeper's digest scheme ACL entries.
 *
 * @param dest Pointer to a zero'ed memory region of at least
 *             LEN_ZK_CREDS + 1 bytes, will contain
 *             the resulting C string after the call.
 */
int creds_for_acl(char *dest, const char *user, const char *pass)
{
	/* 0. cred = "userName:passPhrase"
	 * 1. hash = SHA1(cred)
	 * 2. encd = base64(cred) = base64(SHA1("userName:passPhrase"))
	 * 3. dest = "userName:" + encd
	 */

	char    cred[LEN_ZK_CREDS + 1]  = { 0 };
	uint8_t hash[SHA_DIGEST_LENGTH] = { 0 };
	char    encd[LEN_ZK_CREDS + 1]  = { 0 }; // This could be less.

	/* snprintf() includes \0 in written & counted characters. */
	snprintf(cred, LEN_ZK_CREDS + 1, "%s:%s", user, pass);

	SHA1((const uint8_t*) cred, strlen(cred), hash);

	/* base64() might not terminate the string with \0. */
	base64(encd, hash, LEN_ZK_CREDS, SHA_DIGEST_LENGTH);

	/* snprintf() includes \0 in written & counted characters. */
	snprintf(dest, LEN_ZK_CREDS + 1, "%s:%s", user, encd);

	return SUCCESS;
}

uint32_t zka_connection_get_id()
{
	return sid;
}

//TODO We're checking always against 0 as return value.
// clients should check against SUCCESS
int zka_login()
{
	/* As long as the handle is not NULL, we expect to be connected. */
	if (zk) {
		return SUCCESS;
	}

	if (!srv) {
		fprintf(stderr, "zka_login(): "
				"Name servers have not been set yet.\n");
		return GENERIC_ERROR;
	}

	zoo_set_debug_level(ZK_LOGLEVEL);

	zk = zookeeper_init(srv, NULL, ZK_TIMEOUT, zid, NULL, 0);
	if (!zk) {
		perror("zookeeper_init()");
		fprintf(stderr, "zka_login(): "
				"Could not connect to ZooKeeper.\n");
		return GENERIC_ERROR;
	}

	/* Store client ID for reconnecting after connection loss. */
	zid = zoo_client_id(zk);

	sid++;

	return SUCCESS;
}

int zka_add_auth(Capability cap)
{
	if (!zk && zka_login() != 0) {
		fprintf(stderr, "zka_add_auth(): "
				"Not connected.\n");
		return GENERIC_ERROR;
	}

	char cred[LEN_ZK_CREDS + 1] = { 0 };
	snprintf(cred, LEN_ZK_CREDS, "%s:%s", capability_get_service_name(cap), capability_get_service_pass(cap));

	int zresult = zoo_add_auth(zk, "digest", cred, strlen(cred), NULL, NULL);
	if (zresult != ZOK) {
		fprintf(stderr, "zoo_add_auth(): %s\n", zerror(zresult));
		fprintf(stderr, "zka_add_auth(): "
				"Authentication with ZK failed.\n");
		return GENERIC_ERROR;
	}

	return SUCCESS;
}

void name_servers_set(const char *addresses)
{
	if (srv) {
		zid = NULL; // Different serverlist => force new session.
		free(srv);
	}

	const size_t len = strlen(addresses) + strlen(ZK_CHROOT);
	char* buf  = malloc(len + 1); // + \0
	snprintf(buf, len + 1, "%s%s", addresses, ZK_CHROOT); // + \0
	srv = buf;
}

int zka_assure_connection(const char *func_name)
{
	if (!zk && zka_login() != 0) {
		fprintf(stderr, "%s(): Not connected.\n", func_name);
		return GENERIC_ERROR;
	}

	return SUCCESS;
}

void zka_path_from_name(char path[LEN_SERVICE_NAME + 2], const char *name)
{
	path[0] = '/';
	strncpy(path + 1, name, LEN_SERVICE_NAME + 1); // +1 ensures trailing \0
}

int zka_check_lookup_zresult(int zresult, const char *func_name)
{
	if (zresult == ZNONODE) {
#ifdef DEBUG
		fprintf(stderr, "%s(): Service does not exist.\n", func_name);
#endif
		return NO_SUCH_SERVICE;
	}
	if (zresult != ZOK) {
		fprintf(stderr, "%s(): Failed to get data from ZK.\n", func_name);
		return GENERIC_ERROR;
	}

	return SUCCESS;
}

struct capability *zka_capability_get(Capability cap, const char *func_name)
{
	struct capability *ptr = capability_get(cap);
	if (!ptr) {
		fprintf(stderr, "%s(): Could not look up capability data structure.\n", func_name);
	}

	return ptr;
}

int zka_lookup_once(Capability cap)
{
	int result;
	if ((result = zka_assure_connection("zka_lookup_once")) != SUCCESS) {
		return result;
	}

	struct capability *ptr = zka_capability_get(cap, "zka_lookup_once");
	if (!ptr || check_service_name(ptr->name, "zka_lookup_once") != SUCCESS) {
		return GENERIC_ERROR;
	}

	/* Cap might already have an address string in its buffer. */
	zka_clean_addr(ptr);

	/* ZK expects a path pointing to a ZNode. */
	char path[LEN_SERVICE_NAME + 2]; // Leading '/', trailing '\0'.
	zka_path_from_name(path, ptr->name);

	int length = LEN_SERVICE_ADDR;
	int zresult = zoo_wget(zk, path, NULL, NULL, ptr->addr, &length, NULL);

	return zka_check_lookup_zresult(zresult, "zka_lookup_once");
}

int zka_lookup(Capability cap, void (*callback)(Capability))
{
	int result;
	if ((result = zka_assure_connection("zka_lookup")) != SUCCESS) {
		return result;
	}

	struct capability *ptr = zka_capability_get(cap, "zka_lookup");
	if (!ptr || check_service_name(ptr->name, "zka_lookup") != SUCCESS) {
		return GENERIC_ERROR;
	}

	/* ZK expects a path pointing to a ZNode. */
	char path[LEN_SERVICE_NAME + 2]; // Leading '/', trailing '\0'.
	zka_path_from_name(path, ptr->name);

	struct zka_watcher_context *wctx = malloc(sizeof(struct zka_watcher_context));
	wctx->ptr        = ptr;
	wctx->callback   = callback;
	wctx->is_enabled = 1;

	int zresult = zka_wget(path, ptr, wctx);
	result = zka_check_lookup_zresult(zresult, "zka_lookup");

	/* ZK watch is only set if the request was successful. Especially, this
	 * means that no watch will be set if the node does not exist.
	 */
	if (result == SUCCESS) {
		/* If we have a watcher context for this capability, it is an
		 * old one for the exact same capability that is still active.
		 *
		 * If the watch is triggered we do not want it to perform
		 * automatic update, watch re-setting or callback. We want it to
		 * be deleted quietly if it is triggered.
		 *
		 * If we free it here we cause a SEGFAULT in zka_watcher_node().
		 */
		if (zka_wctxs[cap]) {
			/* Let zka_watcher_node() perform the clean-up. */
			zka_wctxs[cap]->is_enabled = 0;
		}
		zka_wctxs[cap] = wctx;
	} else {
		free(wctx); // Has not been set in ZK nor in array.
	}

	return result;
}

void zka_name_servers_setup()
{
	char creds[LEN_ZK_CREDS + 1] = { 0 };
	creds_for_acl(creds, "guardian", "law");

	struct Id admin = { "digest", creds };

	struct ACL aclList[2];
	aclList[0].perms = ZOO_PERM_CREATE | ZOO_PERM_READ;
	aclList[0].id    = ZOO_ANYONE_ID_UNSAFE;
	aclList[1].perms = ZOO_PERM_ALL;
	aclList[1].id    = admin;

	struct ACL_vector acl;
	acl.count = 2;
	acl.data = aclList;

	if (zk || zka_login() == 0) {
		zoo_create(zk, "/", "Base service directory.", 2, &acl, 0, NULL, 0);
	}
}

int zka_set_address(Capability cap)
{
	const char *const name = capability_get_service_name(cap);
	const char *const addr = capability_get_service_addr(cap);

	/* ZK expects a path pointing to a ZNode. */
	char path[LEN_SERVICE_NAME + 2] = { 0 }; // Leading '/', trailing '\0'.
	path[0] = '/';
	strncat(path, name, LEN_SERVICE_NAME);

	if (!zk && zka_login() != 0) {
		fprintf(stderr, "zka_set_address(): "
				"Not connected.\n");
		return GENERIC_ERROR;
	}

	int zresult = zoo_set(zk, path, addr, strlen(addr), -1);
	if (zresult != ZOK) {
		fprintf(stderr, "zoo_set(): %s\n", zerror(zresult));
		fprintf(stderr, "zka_set_address(): "
				"Failed to write new data to ZK.\n");
		return GENERIC_ERROR;
	}

	return SUCCESS;
}

int zka_create(const char *path, const char *data, struct ACL_vector *acl)
{
	if (!zk && zka_login() != 0) {
		fprintf(stderr, "zka_create(): "
				"Not connected.\n");
		return GENERIC_ERROR;
	}

	int result = SUCCESS;
	int zresult = zoo_create(zk, path, data, strlen(data), acl, 0, NULL, 0);
	if (zresult != ZOK) {
		result = GENERIC_ERROR;
		switch (zresult) {
		case ZNONODE:
			fprintf(stderr, "zoo_create(): %s\n", zerror(zresult));
			fprintf(stderr, "zka_create(): "
					"LibRIPC ZK base node does not exist.\n");
			break;
		case ZNODEEXISTS:
#ifdef DEBUG
			fprintf(stderr, "zoo_create(): %s\n", zerror(zresult));
			fprintf(stderr, "zka_create(): "
					"Service does already exist.\n");
#endif
			result = SERVICE_EXISTS;
			break;
		default:
			fprintf(stderr, "zoo_create(): %s\n", zerror(zresult));
			fprintf(stderr, "zka_create(): "
					"Unhandled ZK error.\n");
			break;
		}
	}

	return result;
}

int zka_service_create(Capability cap)
{
	char creds[LEN_ZK_CREDS + 1] = { 0 };
	creds_for_acl(creds, capability_get_service_name(cap), capability_get_service_pass(cap));

	struct Id id = { "digest", creds };

	struct ACL aclList[2];
	aclList[0].perms = ZOO_PERM_READ;
	aclList[0].id    = ZOO_ANYONE_ID_UNSAFE;
	aclList[1].perms = ZOO_PERM_READ | ZOO_PERM_WRITE;
	aclList[1].id    = id;

	struct ACL_vector acl;
	acl.count = 2;
	acl.data = aclList;

	/* ZK expects a path pointing to a ZNode. */
	char path[LEN_SERVICE_NAME + 2] = { 0 }; // Leading '/', trailing '\0'.
	path[0] = '/';
	strncat(path, capability_get_service_name(cap), LEN_SERVICE_NAME);

	int result = zka_create(path, capability_get_service_addr(cap), &acl);
	if (result == SERVICE_EXISTS) {
#ifdef DEBUG
		fprintf(stderr, "zka_service_create(): "
				"Service does already exist.\n");
#endif
		return result;
	} else if (result != SUCCESS) {
		fprintf(stderr, "zka_service_create(): "
				"Failed to create service.\n");
		return result;
	}

	return result;
}

int zka_disable_updates(Capability cap)
{
	if (cap >= MAX_CAPS) {
		fprintf(stderr, "zka_disable_updates(): "
				"Array index out of bounds.\n");
		return GENERIC_ERROR;
	}

	struct zka_watcher_context *wctx = zka_wctxs[cap];
	if (!wctx) {
#ifdef DEBUG
		fprintf(stderr, "zka_disable_updates(): "
				"Watcher context for capability is already NULL.\n");
#endif
		return SUCCESS;
	}

	/* Watch is still set in ZK and will cause a notification,
	 * but our *internal* handler won't act on it.
	 */
	wctx->is_enabled = 0;

	return SUCCESS;
}

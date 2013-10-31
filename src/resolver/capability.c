#include <zookeeper/zookeeper.h>
#include <stdio.h>
#include <string.h>
#include "naming.h"
#include "capability.h"
#include "zkadapter.h"
#include "address.h"
#include "base64.h"
#include "netarch_context.h"
#include "netarch.h"


static struct capability *caps[MAX_CAPS];



void strrand(char *destination, size_t length)
{
	/* |base64(s)| = 4 * ceil(|s| / 3) */
	const size_t l_bytes = length / 4 * 3;

	uint8_t bytes[l_bytes];

	size_t i;
	for (i = 0; i < l_bytes; ++i) {
		bytes[i] = rand();
	}

	/* Ensure null-termination and that no left-over bits exist. */
	memset(destination, 0, length);
	base64(destination, bytes, length - 1, l_bytes);
}

Capability capability_of_struct(struct capability *ptr)
{
	size_t i = 0;
	while (i < MAX_CAPS && caps[i] != ptr) ++i;
	return i == MAX_CAPS ? INVALID_CAPABILITY : i;
}

size_t next_free_slot()
{
	// TODO: freed capability can be reused. we might not want this to happen.
	size_t result = capability_of_struct(NULL);
	DEBUG("next_free_slot() is '%d'", result);
	return result;
}

int capability_exists(Capability cap, const char *func_name)
{
	if (cap >= MAX_CAPS || cap == INVALID_CAPABILITY || !caps[cap]) {
		fprintf(stderr, "%s(): Invalid capability.\n", func_name);
		return GENERIC_ERROR;
	}

	return SUCCESS;
}

struct capability *capability_get(Capability cap)
{
	if (capability_exists(cap, "capability_get") != SUCCESS) {
		return NULL;
	}

	return caps[cap];
}

const char *capability_get_service_name(Capability cap)
{
	if (capability_exists(cap, "capability_get_service_name") != SUCCESS) {
		return NULL;
	}

	return caps[cap]->name;
}

const char *capability_get_service_pass(Capability cap)
{
	if (capability_exists(cap, "capability_get_service_pass") != SUCCESS) {
		return NULL;
	}

	return caps[cap]->pass;
}

Capability capability_insert(struct capability *ptr)
{
	size_t index = next_free_slot();

	if (index >= MAX_CAPS) {
		fprintf(stderr, "capability_insert(): "
				"No space in internal buffer left.\n");
		return INVALID_CAPABILITY;
	}

	if (index == INVALID_CAPABILITY) {
		fprintf(stderr, "capability_insert(): "
				"Maximum number of capabilities reached.\n");
		return INVALID_CAPABILITY;
	}

	caps[index] = ptr;

	return index;
}

struct capability *capability_allocate()
{
	struct capability *result = malloc(sizeof(struct capability));

	if (!result) {
		fprintf(stderr, "capability_allocate(): "
				"malloc() failed to allocate memory.\n");
		return NULL;
	}

	DEBUG("Allocated memory, now initializing it.");

	memset(result, 0, sizeof(struct capability));

	return result;
}

void capability_free(Capability cap)
{
	if (capability_exists(cap, "capability_free") != SUCCESS) {
		return;
	}

	/* TODO: Add reference counter to structs.
	 *       ripc_receive2() can return an already existing cap,
	 *       probably to another thread.
	 *       ==> unexpected behaviour
	 *
	 * TODO: Caps that are updated automatically have to be passed to
	 *       service_update_disable() before passing them to this method
	 *       (else: SEGFAULT). Fix this by checking zka wctx and doing it
	 *       automatically, if necessary.
	 */
	free(caps[cap]);
	caps[cap] = NULL;
}

Capability capability_create_empty()
{
	struct capability *ptr = capability_allocate();
	if (!ptr) {
		fprintf(stderr, "capability_create_empty(): "
				"Could not allocate capability.\n");
		return INVALID_CAPABILITY;
	}

	Capability result = capability_insert(ptr);
	if (result == INVALID_CAPABILITY) {
		fprintf(stderr, "capability_create_empty(): "
				"Could not store capability in internal buffer.\n");
		free(ptr);
		return INVALID_CAPABILITY;
	}

	return result;
}

Capability capability_from_sender(const char* sendername, struct netarch_address_record *data)
{
	/* Do not create another capability if we already have one. */
	Capability i = 0;
	for (i = 0; i < MAX_CAPS; ++i) {
		struct capability *ptr = caps[i];
		if (!ptr) continue; /* Skip unused capabilities. */
		if (ptr->recv) continue; /* Sender caps NEVER have admin rights. */
		if (strcmp(ptr->name, sendername) != 0) continue; /* Names must match. */
		if (!ptr->send /* Address must match. */
		|| ptr->send->na.lid != data->lid
		|| ptr->send->na.qp_num != data->qp_num) continue;

		break; /* Found one. */
	}

	DEBUG("Loop terminated with i='%d'", i);

	if (i < MAX_CAPS && i != INVALID_CAPABILITY) {
		return i;
	}

	DEBUG("Did not find an existing capability for '%s' with lid='%d' and qpn='%d'",
			sendername, data->lid, data->qpn);

	Capability cap = capability_create_empty();
	if (cap == INVALID_CAPABILITY) {
		DEBUG("Could not allocate local capability for sender '%s'.\n", sendername);
		return INVALID_CAPABILITY;
	}

	strncpy(caps[cap]->name, sendername, LEN_SERVICE_NAME);
	if (netarch_store_sendctx_in_cap(caps[cap], data) != SUCCESS) {
		ERROR("Fail!"); // TODO
		return INVALID_CAPABILITY;
	}

	DEBUG("Now have capability '%d'=>'%s'", cap, capability_get_service_name(cap));

	return cap;
}

Capability capability_create(const char* servicename)
{
	Capability cap = capability_create_empty();
	if (cap == INVALID_CAPABILITY) {
		fprintf(stderr, "capability_create(\"%s\"): "
				"Could not allocate new capability.\n",
				servicename);
		return INVALID_CAPABILITY;
	}

	strncpy(caps[cap]->name, servicename, LEN_SERVICE_NAME);
	strrand(caps[cap]->pass, LEN_SERVICE_PASS);
	capability_set_recvctx(cap);
	// TODO: Currently done by recvctx
	/* capability_set_sendctx(caps[cap]); */

	return cap;
}

int capability_serialize(Capability cap, const char *filename)
{
	if (capability_exists(cap, "capability_serialize") != SUCCESS) {
		return GENERIC_ERROR;
	}

	FILE *storage = fopen(filename, "w");
	if (!storage) {
		perror("fopen()");
		fprintf(stderr, "capability_serialize(%i, \"%s\"): "
				"Failed to open file.\n",
				cap, filename);
		return -1;
	}

	fprintf(storage, "%s:%s", caps[cap]->name, caps[cap]->pass);
	if (ferror(storage)) {
		perror("fprintf()");
		fprintf(stderr, "capability_serialize(%i, \"%s\"): "
				"Failed to write correct information.\n",
				cap, filename);
		fclose(storage);
		return -1;
	}

	fclose(storage);
	return 0;
}

Capability capability_deserialize(const char *filename)
{
	Capability result = INVALID_CAPABILITY;

	FILE *storage = fopen(filename, "r");
	if (!storage) {
		perror("fopen()");
		fprintf(stderr, "capability_deserialize(\"%s\"): "
				"Failed to open file.\n",
				filename);
		return result;
	}

	result = capability_create_empty();
	if (result == INVALID_CAPABILITY) {
		fprintf(stderr, "capability_deserialize(\"%s\"): "
				"Failed to create a capability.\n",
				filename);
		fclose(storage);
		return result;
	}

	fscanf(storage, "%[^:]:%s", caps[result]->name, caps[result]->pass);
	if (ferror(storage)) {
		perror("fscanf()");
		fprintf(stderr, "capability_deserialize(\"%s\"): "
				"Failed to read file contents.\n",
				filename);

		capability_free(result);
		result = INVALID_CAPABILITY;

		fclose(storage);
		return result;
	}

	fclose(storage);
	return result;
}

void capability_debug(Capability cap)
{
	if (capability_exists(cap, "capability_debug") != SUCCESS) {
		return;
	}

	struct capability *c = caps[cap];
	printf("caps[%i] = {\"%s\", \"%s\", %i, %p, %p}\n",
			cap, c->name, c->pass, c->auth_id, c->send, c->recv);
}

int capability_auth(Capability cap)
{
	if (capability_exists(cap, "capability_auth") != SUCCESS) {
		return GENERIC_ERROR;
	}

	/* Has the client performed an authentication for this capability,
	 * and, if so, has it been performed on the current session?
	 */
	uint8_t auth = caps[cap]->auth_id;
	if (auth && auth == zka_connection_get_id()) {
		/* Client already authenticated for that capability. */
		return SUCCESS;
	}

	/* Not authenticated / authentication expired. */

	int zresult = zka_add_auth(cap);
	if (zresult != SUCCESS) {
		fprintf(stderr, "capability_auth(): "
				"Authentication with ZK failed.\n");
		return GENERIC_ERROR;
	}

	caps[cap]->auth_id = zka_connection_get_id();

	return SUCCESS;
}

/**
 * Sets local address for receiving messages.
 */
int capability_set_recvctx(Capability cap)
{
	DEBUG("For cap '%d'.", cap);

	if (capability_exists(cap, "capability_set_recvctx") != SUCCESS) {
		return GENERIC_ERROR;
	}

	struct capability *ptr = capability_get(cap);

	DEBUG("Creating contexts to hold queue state.", cap);
	ptr->send = malloc(sizeof(struct context_sending));
	ptr->recv = malloc(sizeof(struct context_receiving));
	memset(ptr->send, 0, sizeof(struct context_sending));
	memset(ptr->recv, 0, sizeof(struct context_receiving));

	DEBUG("Allocating queue state for cap '%d'.", cap);
	/* FIXME: alloc_queue_state2() must not re-alloc if already existing. */
	alloc_queue_state2(caps[cap]);

	DEBUG("Done.", cap);

	return SUCCESS;
}

/* int capability_set_sendctx(Capability cap) */
/* { */
	/* if (capability_exists(cap, "capability_set_sendctx") != SUCCESS) { */
		/* return GENERIC_ERROR; */
	/* } */

	/* fprintf(stderr, "FIXME: missing capability_set_sendctx() \n"); */

	/* return SUCCESS; */
/* } */

int capability_clear_recvctx(Capability cap)
{
	if (capability_exists(cap, "capability_clear_recvctx") != SUCCESS) {
		return GENERIC_ERROR;
	}

	fprintf(stderr, "FIXME: missing dealloc_queue_state() \n");

	return SUCCESS;
}


int capability_clear_sendctx(struct capability *ptr)
{
	/* if (capability_exists(cap, "capability_clear_sendctx") != SUCCESS) { */
		/* return GENERIC_ERROR; */
	/* } */

	fprintf(stderr, "FIXME: missing capability_clear_sendctx() \n");

	return SUCCESS;
}

int capability_set_name(Capability cap, const char *name)
{
	if (capability_exists(cap, "capability_set_name") != SUCCESS) {
		return GENERIC_ERROR;
	}
	if (strlen(name) > LEN_SERVICE_NAME) {
		fprintf(stderr, "capability_set_name(): "
				"Service name \"%s\" is too long.",
				name);
		return GENERIC_ERROR;
	}

	strncpy(caps[cap]->name, name, LEN_SERVICE_NAME);
	return SUCCESS;
}

int capability_contains_authinfo(Capability cap, const char *func_name)
{
	if (capability_exists(cap, func_name) != SUCCESS) {
		return GENERIC_ERROR;
	}

	const char *const name = caps[cap]->name;
	const char *const pass = caps[cap]->pass;

	if (!name || !strlen(name)) {
		fprintf(stderr, "%s(): %s\n",
				func_name,
				"No service name associated with capability.");
		return GENERIC_ERROR;
	}

	if (!pass || !strlen(pass)) {
		fprintf(stderr, "%s(): %s\n",
				func_name,
				"Capability does not grant this type of access.");
		return GENERIC_ERROR;
	}

	return SUCCESS;
}

int check_service_name(const char *name, const char *func_name)
{
	if (!name) {
		fprintf(stderr, "%s(): Parameter is NULL.\n", func_name);
		return GENERIC_ERROR;
	}

	if (strlen(name) == 0) {
		fprintf(stderr, "%s(): Service name is the empty word.\n", func_name);
		return GENERIC_ERROR;
	}

	return SUCCESS;
}

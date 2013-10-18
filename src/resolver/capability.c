#include <zookeeper/zookeeper.h>
#include <stdio.h>
#include <string.h>
#include "naming.h"
#include "capability.h"
#include "zkadapter.h"
#include "address.h"
#include "base64.h"


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
	return capability_of_struct(NULL);
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

const char *capability_get_service_addr(Capability cap)
{
	if (capability_exists(cap, "capability_get_service_addr") != SUCCESS) {
		return NULL;
	}

	return caps[cap]->addr;
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

	memset(result, 0, sizeof(struct capability));

	return result;
}

void capability_free(Capability cap)
{
	if (capability_exists(cap, "capability_free") != SUCCESS) {
		return;
	}

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
	strncpy(caps[cap]->addr, address_get(), LEN_SERVICE_ADDR);

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
	printf("caps[%i] = {\"%s\", \"%s\", \"%s\", %i}\n",
			cap, c->name, c->pass, c->addr, c->auth_id);
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

int capability_set_address(Capability cap, const char *address)
{
	if (capability_exists(cap, "capability_set_address") != SUCCESS) {
		return GENERIC_ERROR;
	}
	if (strlen(address) > LEN_SERVICE_ADDR) {
		fprintf(stderr, "capability_set_address(): "
				"Address \"%s\" is too long.",
				address);
		return GENERIC_ERROR;
	}

	strncpy(caps[cap]->addr, address, LEN_SERVICE_ADDR);
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

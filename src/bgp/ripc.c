#include <time.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "ripc.h"
#include "common.h"
#include "memory.h"
#include "resolver.h"
#include "resources.h"

struct library_context context;

pthread_mutex_t services_mutex, remotes_mutex;

uint8_t init(void) {
	return true;
}


uint8_t ripc_register_service_id(int service_id) {

	init();
	DEBUG("Allocating service ID %u", service_id);
	return true;
}

uint8_t
ripc_send_short(
		uint16_t src,
		uint16_t dest,
		void **buf,
		size_t *length,
		uint32_t num_items,
		void **return_bufs,
		size_t *return_buf_lengths,
		uint32_t num_return_bufs) {

	DEBUG("Starting short send: %u -> %u (%u items)", src, dest, num_items);
	return ret;
}

uint8_t
ripc_send_long(
		uint16_t src,
		uint16_t dest,
		void **buf,
		size_t *length,
		uint32_t num_items,
		void **return_bufs,
		size_t *return_buf_lengths,
		uint32_t num_return_bufs) {

	DEBUG("Starting long send: %u -> %u (%u items)", src, dest, num_items);

	return 0;
}

uint8_t
ripc_receive(
		uint16_t service_id,
		uint16_t *from_service_id,
		void ***short_items,
		uint32_t **short_item_sizes,
		uint16_t *num_short_items,
		void ***long_items,
		uint32_t **long_item_sizes,
		uint16_t *num_long_items) {

	return 0;
}

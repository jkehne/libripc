#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ripc.h"
#include "config.h"
#include "common.h"

int main(void) {
	int i;

	ripc_register_service_id(SERVER_SERVICE_ID);
	void **short_items = NULL, **long_items = NULL;
	size_t length[WORDS_PER_PACKET];
	for (i = 0; i < WORDS_PER_PACKET; ++i)
		length[i] = PACKET_SIZE;

	void *return_buf_array[SERVER_RETURN_BUFFERS];
	size_t return_buf_length_array[SERVER_RETURN_BUFFERS];

	int count = 0;
	uint16_t from, num_short, num_long;
	uint32_t *short_sizes, *long_sizes;

	return_buf_array[0] = ripc_buf_alloc(PACKET_SIZE * SERVER_RETURN_BUFFERS);
	if (return_buf_array[0]) {
		memset(return_buf_array[0], 0, PACKET_SIZE * SERVER_RETURN_BUFFERS);
		return_buf_length_array[0] = PACKET_SIZE;
		for (i = 1; i < SERVER_RETURN_BUFFERS; ++i) {
			return_buf_length_array[i] = PACKET_SIZE;
			return_buf_array[i] = return_buf_array[0] + i * PACKET_SIZE;
		}
	}

	printf("Starting loop\n");
	while(true) {
		DEBUG("Waiting for message");

		ripc_receive(
				SERVER_SERVICE_ID,
				&from,
				&short_items,
				&short_sizes,
				&num_short,
				&long_items,
				&long_sizes,
				&num_long);

		DEBUG("Received message: %d\n", *(int *)short_items[0]);
		//printf("pingpong %d\n", ++count);

		ripc_send_short(
				SERVER_SERVICE_ID,
				from,
				short_items,
				length,
				num_short,
				return_buf_array,
				return_buf_length_array,
				SERVER_RETURN_BUFFERS);

		/*
		 *  return receive buffer to pool. Note that we only have to free one
		 *  item of the array here, as all array elements are in the same
		 *  receive buffer.
		 */
		ripc_buf_free(short_items[0]);
		free(short_items); //frees the array itself
	}
	return EXIT_SUCCESS;
}

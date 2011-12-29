#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ripc.h"
#include "config.h"
#include "common.h"

int main(void) {
	uint32_t i;

	ripc_register_service_id(SERVER_SERVICE_ID);
	void **short_items = NULL, **long_items = NULL;
	size_t length[WORDS_PER_PACKET];
	void *return_buf_array[NUM_RETURN_BUFFERS];
	size_t return_buf_length_array[NUM_RETURN_BUFFERS];

	for (i = 0; i < WORDS_PER_PACKET; ++i)
		length[i] = PACKET_SIZE;
	int count = 0;
	uint16_t from, num_short, num_long;

	void *recv_buffers[WORDS_PER_PACKET];
	recv_buffers[0] = ripc_buf_alloc(PACKET_SIZE * WORDS_PER_PACKET);
	for (i = 1; i < WORDS_PER_PACKET; ++i) {
		recv_buffers[i] = recv_buffers[0] + i * PACKET_SIZE;
	}

	return_buf_array[0] = ripc_buf_alloc(PACKET_SIZE * NUM_RETURN_BUFFERS);
	if (return_buf_array[0]) {
		memset(return_buf_array[0], 0, PACKET_SIZE * NUM_RETURN_BUFFERS);
		return_buf_length_array[0] = PACKET_SIZE;
		for (i = 1; i < NUM_RETURN_BUFFERS; ++i) {
			return_buf_length_array[i] = PACKET_SIZE;
			return_buf_array[i] = return_buf_array[0] + i * PACKET_SIZE;
		}
	}

	printf("Starting loop\n");
	while(true) {
		DEBUG("Posting receive windows");
		for (i = 0; i < WORDS_PER_PACKET; ++i) {
			ripc_reg_recv_window(recv_buffers[i], PACKET_SIZE);
		}

		DEBUG("Waiting for message");
		ripc_receive(
				SERVER_SERVICE_ID,
				&from,
				&short_items,
				&num_short,
				&long_items,
				&num_long);

		DEBUG("Received message: %d\n", *(int *)long_items[0]);
		//printf("pingpong %d\n", ++count);

		ripc_send_long(
				SERVER_SERVICE_ID,
				from,
				long_items,
				length,
				num_long,
				return_buf_array,
				return_buf_length_array,
				NUM_RETURN_BUFFERS);

		free(long_items); //frees the array itself
	}
	return EXIT_SUCCESS;
}

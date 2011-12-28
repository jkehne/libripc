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
	int length[WORDS_PER_PACKET];
	for (i = 0; i < WORDS_PER_PACKET; ++i)
		length[i] = PACKET_SIZE;

	int count = 0;
	uint16_t from, num_short, num_long;

	printf("Starting loop\n");
	while(true) {
		DEBUG("Waiting for message");

		ripc_receive(
				SERVER_SERVICE_ID,
				&from,
				&short_items,
				&num_short,
				&long_items,
				&num_long);

		DEBUG("Received message: %d\n", *(int *)short_items[0]);
		//printf("pingpong %d\n", ++count);

		ripc_send_short(
				SERVER_SERVICE_ID,
				from,
				short_items,
				length,
				num_short);

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

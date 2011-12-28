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

	int num_items, count = 0;
	uint16_t from;

	printf("Starting loop\n");
	while(true) {
		DEBUG("Waiting for message");

		num_items = ripc_receive(
				SERVER_SERVICE_ID,
				&from,
				&short_items,
				&long_items);

		DEBUG("Received message: %d\n", *(int *)short_items[0]);
		//printf("pingpong %d\n", ++count);

		ripc_send_short(
				SERVER_SERVICE_ID,
				from,
				short_items,
				length,
				WORDS_PER_PACKET);

		for (i = 0; i < WORDS_PER_PACKET; ++i)
			ripc_buf_free(short_items[i]); //returns receive buffer to pool
		free(short_items); //frees the array itself
	}
	return EXIT_SUCCESS;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ripc.h"
#include "config.h"
#include "common.h"

#define NUM_ROUNDS 10
#define PACKET_SIZE 2000

int main(void) {
	ripc_register_service_id(1);
	sleep(1);
	int i, len, recvd = 0;

	//for benchmarking
	struct timeval before, after;
	uint64_t before_usec, after_usec, diff;

	//for sending
	void *msg_array[1];
	int length_array[1];

	//for receiving
	void **short_items = NULL, **long_items = NULL;

	gettimeofday(&before, NULL);

	msg_array[0] = ripc_buf_alloc(PACKET_SIZE);
	bzero(msg_array[0], PACKET_SIZE);
	length_array[0] = PACKET_SIZE;
	printf("Starting loop\n");
	for (i = 0; i < NUM_ROUNDS; ++i) {
		if (ripc_send_short(1, 4, msg_array, length_array, 1))
			continue;
		ripc_receive(1, &short_items, &long_items);
		//printf("Received item\n");
		//printf("Message reads: %u\n", *(int *)short_items[0]);
		recvd++;

		ripc_buf_free(short_items[0]);
		free(short_items);
		//sleep(1);
	}

	gettimeofday(&after, NULL);
	before_usec = before.tv_sec * 1000000 + before.tv_usec;
	after_usec = after.tv_sec * 1000000 + after.tv_usec;
	diff = after_usec - before_usec;

	printf("Exchanged %d items in %lu usec (rtt %f usec)\n", recvd, diff, (double)diff / NUM_ROUNDS);
	return EXIT_SUCCESS;
}

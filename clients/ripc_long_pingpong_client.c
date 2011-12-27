#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ripc.h"
#include "config.h"
#include "common.h"

int main(void) {
	uint16_t my_service_id = ripc_register_random_service_id();
	sleep(1);
	int i, j, len, recvd = 0;

	//for benchmarking
	struct timeval before, after;
	uint64_t before_usec, after_usec, diff;

	//for sending
	void *msg_array[WORDS_PER_PACKET];
	int length_array[WORDS_PER_PACKET];

	//for receiving
	void **short_items = NULL, **long_items = NULL;
	uint16_t from;

	gettimeofday(&before, NULL);

	for (i = 0; i < WORDS_PER_PACKET; ++i) {
		msg_array[i] = ripc_buf_alloc(PACKET_SIZE);
		bzero(msg_array[i], PACKET_SIZE);
		length_array[i] = PACKET_SIZE;
	}

	printf("Starting loop\n");

	for (i = 0; i < NUM_ROUNDS; ++i) {
		if (ripc_send_long(my_service_id, 4, msg_array, length_array, WORDS_PER_PACKET))
			continue;
		ripc_receive(my_service_id, &from, &short_items, &long_items);
		//printf("Received item\n");
		//printf("Message reads: %u\n", *(int *)short_items[0]);
		recvd++;
		for (j = 0; j < WORDS_PER_PACKET; ++j)
			ripc_buf_free(long_items[j]);
		free(long_items);
		//sleep(1);
	}

	gettimeofday(&after, NULL);
	before_usec = before.tv_sec * 1000000 + before.tv_usec;
	after_usec = after.tv_sec * 1000000 + after.tv_usec;
	diff = after_usec - before_usec;

	printf("Exchanged %d items in %lu usec (rtt %f usec)\n", recvd, diff, (double)diff / NUM_ROUNDS);
	return EXIT_SUCCESS;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/in.h>
#include <errno.h>
#include "../src/ripc.h"
#include "config.h"
#include "common.h"

int main(void) {
#ifndef CLIENT_SERVICE_ID
	uint16_t my_service_id = ripc_register_random_service_id();
#else
	uint16_t my_service_id = CLIENT_SERVICE_ID;
	ripc_register_service_id(my_service_id);
#endif
	sleep(1);
	int i, j, len, recvd = 0;
	uint16_t num_short, num_long;

	//for benchmarking
	struct timeval before, after;
	uint64_t before_usec, after_usec, diff;

	//for sending
	void *msg_array[1];
	size_t length_array[1];
	char *request = "GET /data.txt HTTP/1.0\n\n";
	int length; //for sockets

	//for receiving
	void **short_items = NULL, **long_items = NULL;
	uint32_t *short_sizes, *long_sizes;
	uint16_t from;
	void *recv_buf = malloc(1048576);
	void *header_window = ripc_buf_alloc(104857600);
	void *payload_window = ripc_buf_alloc(104857600);

	msg_array[0] = ripc_buf_alloc(strlen(request)+10);
	strcpy((char *)msg_array[0], request);
	length_array[0] = strlen(request)+10;

	printf("Benchmarking RIPC\n");

	recvd = 0;
	gettimeofday(&before, NULL);

	for (i = 0; i < NUM_ROUNDS; ++i) {

		if (ripc_send_short(
				my_service_id,
				76,
				msg_array,
				length_array,
				1,
				NULL,
				NULL,
				0))
			continue;

		ripc_reg_recv_window(header_window, 104857600);
		ripc_reg_recv_window(payload_window, 104857600);

		ripc_receive(
				my_service_id,
				&from,
				&short_items,
				&short_sizes,
				&num_short,
				&long_items,
				&long_sizes,
				&num_long);

		//printf("%s\n", (char *)long_items[0]);
		//printf("%s\n", (char *)long_items[1]);
		//printf("Received item\n");
		//DEBUG("Message reads: %u\n", *(int *)short_items[0]);
		recvd++;

		/*
		 *  return receive buffer to pool. Note that we only have to free one
		 *  item of the array here, as all array elements are in the same
		 *  receive buffer.
		 */

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

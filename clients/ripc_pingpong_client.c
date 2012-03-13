/*  Copyright 2011, 2012 Jens Kehne
 *  Copyright 2012 Jan Stoess, Karlsruhe Institute of Technology
 *
 *  LibRIPC is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License as published by the
 *  Free Software Foundation, either version 2.1 of the License, or (at your
 *  option) any later version.
 *
 *  LibRIPC is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with libRIPC.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
	void *msg_array[WORDS_PER_PACKET];
	size_t length_array[WORDS_PER_PACKET];
	void *return_buf_array[CLIENT_RETURN_BUFFERS];
	size_t return_buf_length_array[CLIENT_RETURN_BUFFERS];

	//for receiving
	void **short_items = NULL, **long_items = NULL;
	uint32_t *short_sizes, *long_sizes;
	uint16_t from;

	gettimeofday(&before, NULL);

	for (i = 0; i < WORDS_PER_PACKET; ++i) {
		msg_array[i] = ripc_buf_alloc(PACKET_SIZE);
		memset(msg_array[i], 0, PACKET_SIZE);
		//strcpy((char *)msg_array[i], "Hello short world!");
		length_array[i] = PACKET_SIZE;
	}

	return_buf_array[0] = ripc_buf_alloc(PACKET_SIZE * CLIENT_RETURN_BUFFERS);
	if (return_buf_array[0]) {
		memset(return_buf_array[0], 0, PACKET_SIZE * CLIENT_RETURN_BUFFERS);
		return_buf_length_array[0] = PACKET_SIZE;
		for (i = 1; i < CLIENT_RETURN_BUFFERS; ++i) {
			return_buf_length_array[i] = PACKET_SIZE;
			return_buf_array[i] = return_buf_array[0] + i * PACKET_SIZE;
		}
	}

	printf("Starting loop\n");

	for (i = 0; i < NUM_ROUNDS; ++i) {

		*(int *)msg_array[0] = i;

		if (ripc_send_short(
				my_service_id,
				SERVER_SERVICE_ID,
				msg_array,
				length_array,
				WORDS_PER_PACKET,
				return_buf_array,
				return_buf_length_array,
				CLIENT_RETURN_BUFFERS))
			continue;

		ripc_receive(
				my_service_id,
				&from,
				&short_items,
				&short_sizes,
				&num_short,
				&long_items,
				&long_sizes,
				&num_long);

		//printf("Received item\n");
		DEBUG("Message reads: %u\n", *(int *)short_items[0]);
		recvd++;

		/*
		 *  return receive buffer to pool. Note that we only have to free one
		 *  item of the array here, as all array elements are in the same
		 *  receive buffer.
		 */
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

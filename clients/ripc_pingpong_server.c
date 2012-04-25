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
	int i;

	ripc_register_service_id(SERVER_SERVICE_ID);
	void **short_items = NULL, **long_items = NULL;
//	size_t length[WORDS_PER_PACKET];
//	for (i = 0; i < WORDS_PER_PACKET; ++i)
//		length[i] = PACKET_SIZE;
	sleep(1);

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
                uint8_t ret = ripc_receive(
                        SERVER_SERVICE_ID,
                        &from,
                        &short_items,
                        &short_sizes,
                        &num_short,
                        &long_items,
                        &long_sizes,
                        &num_long);
                
                
		if (ret)
                        printf("Failed receiving message: %d\n", ret);
                else
                        DEBUG("Received message: %d\n", *(int *)short_items[0]);
		//printf("pingpong %d\n", ++count);

		ripc_send_short(
                        SERVER_SERVICE_ID,
                        from,
                        short_items,
                        (size_t *)short_sizes,
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ripc.h"
#include "config.h"
#include "common.h"

#define PACKET_SIZE 20000000

int main(void) {
	ripc_register_service_id(4);
	void **short_items = NULL, **long_items = NULL;
	int length = PACKET_SIZE;
	int num_items, count = 0;
	uint16_t from;

	printf("Starting loop\n");
	while(true) {
		DEBUG("Waiting for message");
		num_items = ripc_receive(4, &from, &short_items,&long_items);
		DEBUG("Received message: %d\n", *(int *)long_items[0]);
		//printf("pingpong %d\n", ++count);
		ripc_send_long(4, from, long_items, &length, 1);
		ripc_buf_free(long_items[0]); //returns receive buffer to pool
		free(long_items); //frees the array itself
	}
	return EXIT_SUCCESS;
}

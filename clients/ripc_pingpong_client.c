#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ripc.h"
#include "config.h"
#include "common.h"

#define NUM_ROUNDS 10000

int main(void) {
	ripc_register_service_id(1);
	int i, len, recvd = 0;

	//for sending
	void *msg_array[1];
	msg_array[0] = ripc_buf_alloc(sizeof(i));
	int length_array[1];

	//for receiving
	void **short_items = NULL, **long_items = NULL;

	for (i = 0; i < NUM_ROUNDS; ++i) {
		*(int *)msg_array[0] = i;
		length_array[0] = sizeof(i);
		if (ripc_send_short(1, 4, msg_array, length_array, 1))
			continue;
		ripc_receive(1, &short_items, &long_items);
		//printf("Received item\n");
		//printf("Message reads: %u\n", *(int *)short_items[0]);
		if (*(int *)short_items[0] != i) {
			panic("FAILED!");
		} else
			recvd++;
		ripc_buf_free(short_items[0]);
		free(short_items);
		//sleep(1);
	}
	printf("Exchanged %d items\n", recvd);
	return EXIT_SUCCESS;
}

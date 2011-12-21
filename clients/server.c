#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ripc.h"
#include "config.h"
#include "common.h"

int main(void) {
	ripc_register_service_id(4);
	void **short_items = NULL, **long_items = NULL;
	int num_items;
	uint16_t from;

	while(true) {
		DEBUG("Waiting for message");
		num_items = ripc_receive(4, &from, &short_items,&long_items);
		printf("Received message: %d: %s\n", *(int *)short_items[0], (char *)short_items[1]);
		ripc_buf_free(short_items[0]);
	}
	return EXIT_SUCCESS;
}

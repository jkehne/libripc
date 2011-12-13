#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ripc.h"
#include "config.h"
#include "common.h"

int main(void) {
	ripc_register_service_id(4);
	void **short_items = NULL, **long_items = NULL;
	while(true) {
		DEBUG("Waiting for message");
		ripc_receive(4, &short_items,&long_items);
		printf("Received message: %s\n",(char *)short_items[0]);
		ripc_buf_free(short_items[0]);
	}
	return EXIT_SUCCESS;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ripc.h"
#include "config.h"
#include "common.h"

int main(void) {
	ripc_register_service_id(4);
	void **short_items, **long_items;
	DEBUG("Waiting for message");
	ripc_receive(4, short_items,long_items);
	return EXIT_SUCCESS;
}

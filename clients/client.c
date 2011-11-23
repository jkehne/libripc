#include <stdio.h>
#include <stdlib.h>
#include "../src/ripc.h"
#include "common.h"

int main(void) {
	ripc_register_service_id(1);
	ripc_register_service_id(2);
	ripc_register_random_service_id();
	return EXIT_SUCCESS;
}

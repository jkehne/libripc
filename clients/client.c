#include <stdio.h>
#include <stdlib.h>
#include "../src/ripc.h"
#include "common.h"

int main(void) {
	ripc_register_service_id(1);
	return EXIT_SUCCESS;
}

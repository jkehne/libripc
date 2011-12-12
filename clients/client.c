#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ripc.h"
#include "config.h"
#include "common.h"

int main(void) {
	ripc_register_service_id(1);
	char *msg = "Hello World!";
	ripc_send_short(1, 4, (void *)msg, strlen(msg));
	return EXIT_SUCCESS;
}

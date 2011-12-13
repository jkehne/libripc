#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ripc.h"
#include "config.h"
#include "common.h"

int main(void) {
	ripc_register_service_id(1);
	int i;
	char *msg = "Hello World!";
	char *tmp_msg = ripc_buf_alloc(strlen(msg) + 10);
	for (i = 0; true; ++i) {
		sprintf(tmp_msg, "%u: %s", i, msg);
		ripc_send_short(1, 4, (void *)tmp_msg, strlen(tmp_msg));
		sleep(1);
	}
	return EXIT_SUCCESS;
}

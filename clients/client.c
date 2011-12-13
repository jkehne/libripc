#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ripc.h"
#include "config.h"
#include "common.h"

int main(void) {
	ripc_register_service_id(1);
	int i, len;
	char *msg = "Hello World!";
	char *tmp_msg = ripc_buf_alloc(strlen(msg) + 10);
	for (i = 0; true; ++i) {
		sprintf(tmp_msg, "%u: %s", i, msg);
		len = strlen(tmp_msg);
		ripc_send_short(1, 4, (void **)&tmp_msg, &len, 1);
		sleep(1);
	}
	return EXIT_SUCCESS;
}

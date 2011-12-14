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
	void *msg_array[2];
	msg_array[0] = ripc_buf_alloc(sizeof(i));
	msg_array[1] = ripc_buf_alloc(strlen(msg));
	int length_array[2];
	for (i = 0; true; ++i) {
		*(int *)msg_array[0] = i;
		length_array[0] = sizeof(i);
		//msg_array[1] = (void *)msg;
		strcpy((char *)msg_array[1],msg);
		length_array[1] = strlen(msg);
		ripc_send_short(1, 4, msg_array, length_array, 2);
		sleep(1);
	}
	return EXIT_SUCCESS;
}

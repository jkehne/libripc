/*  Copyright 2011, 2012 Jens Kehne
 *  Copyright 2012 Jan Stoess, Karlsruhe Institute of Technology
 *
 *  LibRIPC is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License as published by the
 *  Free Software Foundation, either version 2.1 of the License, or (at your
 *  option) any later version.
 *
 *  LibRIPC is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with libRIPC.  If not, see <http://www.gnu.org/licenses/>.
 */
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
	size_t length_array[2];
	for (i = 0; true; ++i) {
		*(int *)msg_array[0] = i;
		length_array[0] = sizeof(i);
		//msg_array[1] = (void *)msg;
		strcpy((char *)msg_array[1],msg);
		length_array[1] = strlen(msg);
		ripc_send_short(1, 4, msg_array, length_array, 2, NULL, NULL, 0);
		sleep(1);
	}
	return EXIT_SUCCESS;
}

/*  Copyright 2011, 2012 Jens Kehne
 *  Copyright 2012 Jan Stoess, Karlsruhe Institute of Technology
 *  Copyright 2013 Andreas Waidler
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
#include <pthread.h>


int main(int argc, char *argv[])
{
	/* if (argc != 1 */
	/* || (strcmp(argv[1], "old") != 0 && strcmp(argv[1], "new") != 0)) { */
		/* fprintf(stderr, "%s: Requires \"old\" or \"new\" as parameter.\n", argv[0]); */
		/* return 1; */
	/* } */

	/* bool isOldResolver = strcmp(argv[1], "old") == 0; */

	ripc_init();
	sleep(1);

	fprintf(stdout, "LibRIPC resolver benchmark.\n");

#ifdef OLD_RESOLVER
	uint16_t my_service_id = ripc_register_random_service_id();
#else
	char name[1024] = { 0 }; // Our process name.
	Capability us = INVALID_CAPABILITY;

	snprintf(name, 1023, "User%d", rand() % 100);
	us = capability_create(name);
#endif


	void *msg_array[1];
	uint32_t length_array[1];
	uint32_t packet_size = 4;
	char input[1024];

	msg_array[0] = ripc_buf_alloc(packet_size);
	memset(msg_array[0], 0, packet_size);

	*((uint32_t*) msg_array[0]) = (uint32_t) 42;
	length_array[0] = packet_size;

	uint32_t i = 0;
	for (i = 0; i < 10000; ++i) {
#ifdef OLD_RESOLVER
		int result = ripc_send_short(
			my_service_id,
			SERVER_SERVICE_ID,
			msg_array,
			length_array,
			1,
			NULL,
			NULL,
			0);
#else
		Capability srv;
		if ((srv = service_lookup(XCHANGE_SERVICE, NULL)) == INVALID_CAPABILITY) {
			fprintf(stderr, "--- Lookup failed, exitting.\n");
			return 1;
		}

		int result = ripc_send_short2(
			us,
			srv,
			msg_array,
			length_array,
			1,
			NULL,
			NULL,
			0);
#endif
	}

	ripc_buf_free(msg_array[0]);

	return 0;

	/* int i, len; */
	/* char *msg = "Hello World!"; */
	/* char *tmp_msg = ripc_buf_alloc(strlen(msg) + 10); */
	/* void *msg_array[2]; */
	/* msg_array[0] = ripc_buf_alloc(sizeof(i)); */
	/* msg_array[1] = ripc_buf_alloc(strlen(msg)); */
	/* size_t length_array[2]; */
	/* for (i = 0; true; ++i) { */
		/* *(int *)msg_array[0] = i; */
		/* length_array[0] = sizeof(i); */
		/* //msg_array[1] = (void *)msg; */
		/* strcpy((char *)msg_array[1],msg); */
		/* length_array[1] = strlen(msg); */
		/* ripc_send_short(1, 4, msg_array, length_array, 2, NULL, NULL, 0); */
		/* sleep(1); */
	/* } */
	/* return EXIT_SUCCESS; */
}


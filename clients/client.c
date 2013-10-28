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

pthread_t display_thread;

void onChange(Capability srv)
{
	fprintf(stderr, "!!! Service migrated: ");
	capability_debug(srv);
}

Capability us = INVALID_CAPABILITY;

void *display(void *arg)
{
	Capability srv;

	while(true) {
		void **short_items = NULL, **long_items = NULL;
		uint32_t *short_sizes, *long_sizes;
		int num_items;
		uint16_t num_short, num_long;

		ripc_receive2(
				us,
				&srv,
				&short_items,
				&short_sizes,
				&num_short,
				&long_items,
				&long_sizes,
				&num_long);

		printf("%s\n", (char *)short_items[0]);

		ripc_buf_free(short_items[0]);
	}
}

int main(void)
{
	ripc_init();
	sleep(1);

	fprintf(stderr, "LibRIPC simple message sending client.\n");

	fprintf(stderr, "--- Creating capability for local process.\n");
	us = capability_create("RandomClientProcess");
	if (us == INVALID_CAPABILITY) {
		fprintf(stderr, "--- Could not create capability for us.\n");
		return 1;
	}

	fprintf(stderr, "--- Okay, so we are: "); capability_debug(us);


	fprintf(stderr, "--- Looking up destination service \"%s\".\n", XCHANGE_SERVICE);

	Capability srv;
	while ((srv = service_lookup(XCHANGE_SERVICE, onChange)) == INVALID_CAPABILITY) {
		const uint8_t t = 10;
		fprintf(stderr, "--- Lookup failed, trying again in %d seconds.\n", t);
		sleep(t);
	}

	fprintf(stderr, "--- Server found: "); capability_debug(srv);

	pthread_create(&display_thread, NULL, &display, NULL);

	void *msg_array[1];
	uint32_t length_array[1];
	uint32_t packet_size = 100;
	char input[1024];

	do {
		msg_array[0] = ripc_buf_alloc(packet_size);
		memset(msg_array[0], 0, packet_size);

		fprintf(stderr, "> ");
		fgets(input, packet_size, stdin);
		DEBUG("Message to be sent: '%s'", input);

		strncpy((char *)msg_array[0], input, packet_size - 1);
		length_array[0] = strlen(msg_array[0]) + 1; // + \0

		fprintf(stderr, "--- Sending message... \n");

		int result = ripc_send_short2(
			us,
			srv,
			msg_array,
			length_array,
			1,
			NULL,
			NULL,
			0);


		fprintf(stderr, "--- LibRIPC returned '%d'.\n", result);

		ripc_buf_free(msg_array[0]);
	} while (true);

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

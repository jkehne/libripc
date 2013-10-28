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
#include <unistd.h>
#include "../src/ripc.h"
#include "config.h"
#include "common.h"

struct cap_list_node {
	Capability cap;
	struct cap_list_node *next;
} head = { .cap = INVALID_CAPABILITY, .next = NULL };

struct cap_list_node *last = &head;

int main(void) {
	int result = SUCCESS;
	ripc_init();
	sleep(1);

	fprintf(stderr, "LibRIPC message service %s.\n", XCHANGE_SERVICE);

	fprintf(stderr, "--- Trying to deserialize old capability...\n");
	Capability srv = capability_deserialize(XCHANGE_SERVICE);
	if (srv == INVALID_CAPABILITY) {
		fprintf(stderr, "--- Deserialization failed.\n"
				"--- This usually means that the service has not been created yet (or else we've lost data to access it).\n"
				"--- Trying to create that service...\n");

		srv = capability_create(XCHANGE_SERVICE);
		if (srv == INVALID_CAPABILITY) {
			fprintf(stderr, "--- Capability creation failed. This should NEVER happen.\n");
			return -1;
		}

		/* fprintf(stderr, "--- Capability is "); capability_debug(test); */

		/* fprintf(stderr, "--- Serializing it for future use...\n"); */
		if (capability_serialize(srv, XCHANGE_SERVICE)) {
			fprintf(stderr, "--- Capability serialization failed.\n");
			fprintf(stderr, "--- Capability was: "); capability_debug(srv);
			return -1;
		}
		/* fprintf(stderr, "--- Serialized.\n"); */

		/* fprintf(stderr, "--- Trying to register that service...\n"); */
		result = service_create(srv);
		if (result != SUCCESS) {
			unlink(XCHANGE_SERVICE); // Service not registered but cap already serialized.
		}
		if (result == SERVICE_EXISTS) {
			fprintf(stderr, "--- Service does already exists.\n"
					"--- You have to try to recover a capability, a serialized capability, or delete it from ZK manually.\n");
		} else if (result != SUCCESS) {
			fprintf(stderr, "--- Service creation somehow failed. Nothing we can do now.\n");
			return -1;
		}
	}

	fprintf(stderr, "--- Got service capability.\n"
			"--- Is: "); capability_debug(srv);

	if ((result = service_login(srv)) != SUCCESS) {
		fprintf(stderr, "--- Somehow failed to update data in ZK. Diagnose manually.\n");
		return 1;
	}
	fprintf(stderr, "--- Service ready to use: "); capability_debug(srv);

	void **short_items = NULL, **long_items = NULL;
	uint32_t *short_sizes, *long_sizes;
	int num_items;
	uint16_t num_short, num_long;
	Capability pre_from = INVALID_CAPABILITY;
	Capability from;

	while(true) {
		num_items = ripc_receive2(
				srv,
				&from,
				&short_items,
				&short_sizes,
				&num_short,
				&long_items,
				&long_sizes,
				&num_long);

		printf("Received message. "
			"Number of items: '%d' ('%d' s, '%d' l). "
			"Text: '%s'.\n",
			num_items,
			num_short,
			num_long,
			(char *)short_items[0]);

		void *msg_array[1];
		uint32_t length_array[1];
		uint32_t packet_size = 100;

		msg_array[0] = ripc_buf_alloc(packet_size);
		memset(msg_array[0], 0, packet_size);
		snprintf(msg_array[0], packet_size, "%s> %s",
				capability_get_service_name(from),
				(char*) short_items[0]);
		length_array[0] = strlen(msg_array[0]) + 1; // + \0

		struct cap_list_node *it = head.next;
		while (it != NULL) {
			int result = ripc_send_short2(
				srv,
				it->cap,
				msg_array,
				length_array,
				1,
				NULL,
				NULL,
				0);
			it = it->next;
		}

		last->next = malloc(sizeof(struct cap_list_node));
		last->next->cap = from;
		last->next->next = NULL;
		last = last->next;

		ripc_buf_free(short_items[0]);
	}

	/* ripc_buf_free(ack_array[0]); */


//	ripc_register_service_id(4);
//	void **short_items = NULL, **long_items = NULL;
//	uint32_t *short_sizes, *long_sizes;
//	int num_items;
//	uint16_t from, num_short, num_long;
//
//	while(true) {
//		DEBUG("Waiting for message");
//		num_items = ripc_receive(
//				4,
//				&from,
//				&short_items,
//				&short_sizes,
//				&num_short,
//				&long_items,
//				&long_sizes,
//				&num_long);
//		printf("Received message: %d: %s\n", *(int *)short_items[0], (char *)short_items[1]);
//		ripc_buf_free(short_items[0]);
//	}
//	return EXIT_SUCCESS;
}

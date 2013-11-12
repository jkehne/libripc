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
#include "../src/common.h"
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
	Capability srv;
/* # ifndef BENCHMARK_RESOLVER_DISABLE_CACHE */ // Required by update() (AVOID_OVERHEAD) as well
	if ((srv = service_lookup(XCHANGE_SERVICE, NULL)) == INVALID_CAPABILITY) {
		fprintf(stderr, "--- Lookup failed, exitting.\n");
		return 1;
	}
/* # endif */
#endif


	void *msg_array[1];
	uint32_t length_array[1];
	uint32_t packet_size = 4;

	msg_array[0] = ripc_buf_alloc(packet_size);
	memset(msg_array[0], 0, packet_size);

	length_array[0] = packet_size;


	void **short_items = NULL, **long_items = NULL;
	uint32_t *short_sizes, *long_sizes;
	int num_items;
	uint16_t num_short, num_long;

	struct timeval before, after;
	uint64_t before_usec, after_usec, diff;
	gettimeofday(&before, NULL);

	uint32_t i = 0, recvd = 0;
	for (i = 0; recvd < BENCHMARK_NUM_ROUNDS; ++i) {
		*((uint32_t*) msg_array[0]) = (uint32_t) i;
#ifdef OLD_RESOLVER
# ifdef BENCHMARK_RESOLVER_DISABLE_CACHE
		if (context.remotes[SERVER_SERVICE_ID]) {
			free(context.remotes[SERVER_SERVICE_ID]);
			context.remotes[SERVER_SERVICE_ID] = NULL;
		}
# endif
		int result = ripc_send_short(
			my_service_id,
			SERVER_SERVICE_ID,
			msg_array,
			length_array,
			1,
			NULL,
			NULL,
			0);
		if(result) continue;

		uint16_t from;
		num_items = ripc_receive(
				my_service_id,
				&from,
				&short_items,
				&short_sizes,
				&num_short,
				&long_items,
				&long_sizes,
				&num_long);
#else
# ifdef BENCHMARK_RESOLVER_DISABLE_CACHE
#  ifdef BENCHMARK_RESOLVER_AVOID_MALLOC_OVERHEAD
		if ((service_update_once(srv)) != SUCCESS) {
			fprintf(stderr, "--- Update failed, exitting.\n");
			return 1;
		}
#  else
		if ((srv = service_lookup_once(XCHANGE_SERVICE)) == INVALID_CAPABILITY) {
			fprintf(stderr, "--- Lookup failed, exitting.\n");
			return 1;
		}
#  endif
# endif

		int result = ripc_send_short2(
			us,
			srv,
			msg_array,
			length_array,
			1,
			NULL,
			NULL,
			0);
		if(result) continue;

		Capability from;
		num_items = ripc_receive2(
				us,
				&from,
				&short_items,
				&short_sizes,
				&num_short,
				&long_items,
				&long_sizes,
				&num_long);
		// Use sender caching.
		/* capability_free(from); // We already know the server. Avoid buffer overflow. */
# ifdef BENCHMARK_RESOLVER_DISABLE_CACHE
#  ifndef BENCHMARK_RESOLVER_AVOID_MALLOC_OVERHEAD
		capability_free(srv); // Will look it up in next iteration, avoid ovverflow.
#  endif
# endif
#endif
		++recvd;

		ripc_buf_free(short_items[0]);
		free(short_items);
	}

	gettimeofday(&after, NULL);
	before_usec = before.tv_sec * 1000000 + before.tv_usec;
	after_usec = after.tv_sec * 1000000 + after.tv_usec;
	diff = after_usec - before_usec;

	printf("Exchanged %d items in %lu usec (rtt %f usec), %d losses.\n", i, diff, (double)diff / i, i - recvd);

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


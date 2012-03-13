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
	ripc_register_service_id(4);
	void **short_items = NULL, **long_items = NULL;
	uint32_t *short_sizes, *long_sizes;
	int num_items;
	uint16_t from, num_short, num_long;

	while(true) {
		DEBUG("Waiting for message");
		num_items = ripc_receive(
				4,
				&from,
				&short_items,
				&short_sizes,
				&num_short,
				&long_items,
				&long_sizes,
				&num_long);
		printf("Received message: %d: %s\n", *(int *)short_items[0], (char *)short_items[1]);
		ripc_buf_free(short_items[0]);
	}
	return EXIT_SUCCESS;
}

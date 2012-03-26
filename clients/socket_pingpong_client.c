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
#include "config.h"
#include "common.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "../src/ripc.h"

int main(int argc, char *argv[]) {
	int sockfd;
	struct sockaddr_in serv_addr;
	uint32_t i;

	//for benchmarking
	struct timeval before, after;
	uint64_t before_usec, after_usec, diff;
	uint32_t recvd = 0;

	//for sending
	int length;
	uint32_t packet_size = argc > 1 ? atoi(argv[1]) : PACKET_SIZE;
	uint8_t payload[packet_size];
	memset(&payload, 0, packet_size);

	//for receiving

	gettimeofday(&before, NULL);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		panic("Error creating socket");
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(12345);
	serv_addr.sin_addr.s_addr = inet_addr("192.168.10.67");

	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) {
		panic("Error connecting");
	}

	for (i = 0; i < NUM_ROUNDS; ++i) {
		length = write(sockfd, &payload, packet_size);
		length = read(sockfd, &payload, packet_size);

		recvd++;
	}

	gettimeofday(&after, NULL);
	before_usec = before.tv_sec * 1000000 + before.tv_usec;
	after_usec = after.tv_sec * 1000000 + after.tv_usec;
	diff = after_usec - before_usec;

	printf("Exchanged %d items in %lu usec (rtt %f usec)\n", recvd, diff, (double)diff / NUM_ROUNDS);
	return EXIT_SUCCESS;
}

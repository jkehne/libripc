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

int main(void) {
	int listen_sockfd, conn_sockfd;
	int length;
	struct sockaddr_in serv_addr, cli_addr;

	uint8_t payload[PACKET_SIZE];

	listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sockfd < 0) {
		panic("Failed to open socket");
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(12345);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	if(bind(
			listen_sockfd,
			(struct sockaddr *)&serv_addr,
			sizeof(serv_addr)
			)) {
		panic("Failed to bind socket");
	}

	//wait for connection
	listen(listen_sockfd, 5);

	while (1) {
		length = sizeof(cli_addr);
		conn_sockfd = accept(
				listen_sockfd,
				(struct sockaddr *)&cli_addr,
				&length
				);
		if (conn_sockfd < 0) {
			panic("Error on accept");
		}

		while(1) {
			//DEBUG("Waiting for message");
			length = read(conn_sockfd, &payload, PACKET_SIZE);
			if (length < 1)
				break;
			//DEBUG("Received message: %d\n", payload);
			length = write(conn_sockfd, &payload, length);
		}
		close(conn_sockfd);
	}
	return EXIT_SUCCESS;
}

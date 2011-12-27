#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "common.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

int main(void) {
	int sockfd;
	struct sockaddr_in serv_addr;
	uint32_t i;

	//for benchmarking
	struct timeval before, after;
	uint64_t before_usec, after_usec, diff;
	uint32_t recvd = 0;

	//for sending
	int length;
	uint8_t payload[PACKET_SIZE];
	bzero(&payload, PACKET_SIZE);

	//for receiving

	gettimeofday(&before, NULL);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		panic("Error creating socket");
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(12345);
	serv_addr.sin_addr.s_addr = inet_addr("10.0.0.1");

	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) {
		panic("Error connecting");
	}

	for (i = 0; i < NUM_ROUNDS; ++i) {
		length = write(sockfd, &payload, PACKET_SIZE);
		length = read(sockfd, &payload, PACKET_SIZE);

		recvd++;
	}

	gettimeofday(&after, NULL);
	before_usec = before.tv_sec * 1000000 + before.tv_usec;
	after_usec = after.tv_sec * 1000000 + after.tv_usec;
	diff = after_usec - before_usec;

	printf("Exchanged %d items in %lu usec (rtt %f usec)\n", recvd, diff, (double)diff / NUM_ROUNDS);
	return EXIT_SUCCESS;
}

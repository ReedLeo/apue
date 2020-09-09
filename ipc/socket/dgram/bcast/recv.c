#include <asm-generic/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "proto.h"

#define IPSTRSIZE 16

int main(int argc, char** argv)
{
	int sd = 0;	
	int size = 0;
	int optval = 0;
	struct sockaddr_in laddr = {0};
	struct sockaddr_in raddr = {0};
	struct msg_st* rbufp = NULL;
	char ipstr[IPSTRSIZE] = {0};
	socklen_t addr_len = 0;

	size = sizeof(struct msg_st) + NAMEMAX;
	rbufp = malloc(size);
	if (rbufp == NULL)
	{
		perror("malloc()");
		exit(1);
	}

	sd = socket(AF_INET, SOCK_DGRAM, 0/* equivalent to IPPROTO_UDP*/);
	if (sd < 0)
	{
		perror("socket()");
		exit(1);
	}
	
	/* enable the socket to receive boradcast packets */
	optval = 1;
	if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) < 0)
	{
		perror("setsockopt()");
		exit(1);
	}

	laddr.sin_family = AF_INET;
	laddr.sin_port = htons(atoi(RECVPORT));
	inet_pton(AF_INET, "0.0.0.0.0", &laddr.sin_addr);
	addr_len = sizeof(laddr); 
	
	if ( bind(sd, (void*)&laddr, sizeof(laddr)) < 0)
	{
		perror("bind()");
		exit(1);
	}

	while (1)
	{
		/* Used by UDP */
		if (recvfrom(sd, rbufp, size, 0, (void*)&raddr, &addr_len) < 0)
		{
			perror("recvfrom()");
			exit(1);
		}

		if (inet_ntop(AF_INET, &raddr.sin_addr, ipstr, IPSTRSIZE) == NULL)
		{
			perror("inte_ntop()");
			exit(1);
		}

		printf("---MESSAGE FROM %s:%d---\n", ipstr, ntohs(raddr.sin_port));
		printf("-    Name:    %s\n", rbufp->name);
		printf("-    Math:    %3d\n", rbufp->math);
		printf("-    Chinese: %3d\n", rbufp->chinese);
		puts("======");

	}
	
	close(sd);

	return 0;
}

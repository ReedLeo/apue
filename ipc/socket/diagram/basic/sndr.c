#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "proto.h"

int main(int argc, char** argv)
{
	int sd = 0;
	struct sockaddr_in raddr = {0};
	struct msg_st sbuf = {0};

	if (argc < 3)
	{
		perror("Too few arguments.\n");
		exit(1);
	}
	
	strcpy(sbuf.name, argv[2]);
	sbuf.math = atoi(argv[3]);
	sbuf.chinese = atoi(argv[4]);

	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0)
	{
		perror("socket()");
		exit(1);
	}
	
	raddr.sin_family = AF_INET;
	raddr.sin_port = htons(atoi(RECVPORT));
	inet_pton(AF_INET, argv[1], &raddr.sin_addr);

	/*
	if (bind(sd, &laddr, sizeof(laddr)) < 0 )
	{
		perror("bind()");
		exit(1);
	}
	*/
	/* Ommit bind, it will allocate an available one for us */	
	if (sendto(sd, &sbuf, sizeof(sbuf), 0, (void*)&raddr, sizeof(raddr)) < 0)
	{
		perror("send()");
		exit(1);
	}
		
	puts("OK");

	close(sd);
	return 0;
}	

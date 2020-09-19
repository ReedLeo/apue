#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include "proto.h"

int main(int argc, char** argv)
{
	int sd = 0;
	long long stamp;
	FILE* fp = NULL;
	struct sockaddr_in raddr = {0};

	if (argc < 2)
	{
		fprintf(stderr, "Too few arguments.\n");
		exit(1);
	}

	// 1st. create a stream (TCP) socket.
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0)
	{
		perror("socket()");
		exit(1);
	}

	// 2nd. bind can omit
	
	// 3rd. connect to server
	raddr.sin_family = AF_INET;
	raddr.sin_port = htons(atoi(SERVERPORT));
	raddr.sin_addr.s_addr = inet_addr(argv[1]);
	if ( connect(sd, (void*)&raddr, sizeof(raddr)) < 0 )
	{
		perror("connect()");
		exit(1);
	}

	fp = fdopen(sd, "r+");
	if (fp == NULL)
	{
		perror("fdopen()");
		exit(1);
	}

	if ( fscanf(fp, FMT_STAMP, &stamp)  < 1)
	{
		fprintf("Bad stamp fscanf with error: \"%s\"\n", strerror(errno));
	}
	else
	{
		fprintf(stdout, FMT_STAMP, stamp);
	}
	
	fclose(fp);
	close(sd);
	return 0;
}

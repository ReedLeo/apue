#include <asm-generic/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <errno.h>
#include <time.h>
#include <arpa/inet.h>

#include "proto.h"

#define BACKLOG 200
#define BUFSIZE 1024
#define IPSTRSIZE 16

static void serverjob(int sd, const struct sockaddr_in* paddr)
{
	char buf[BUFSIZE] = {0};
	int len = 0;
	len = snprintf(buf, BUFSIZE, FMT_STAMP, (long long)time(NULL));
	if (len < 0)
	{
		perror("snprintf()");
		exit(1);
	}
	
	while (send(sd, buf, len, 0) < 0)
	{
		if (errno == EINTR)
			continue;
		perror("send()");
		exit(1);
	}
	return;
}

int main(int argc, char** argv)
{
	int sd = 0;
	struct sockaddr_in laddr = {0};
	struct sockaddr_in raddr = {0};
	socklen_t raddr_len;
	char ipstr[IPSTRSIZE] = {0};
	int val = 0;	
	
	// 1st. get socket fd
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0)
	{
		perror("socket()");
		exit(1);
	}

	val = 1;
	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0)
	{
		perror("setsockopt()");
		exit(1);
	}
	// 2nd. initialize local sockaddr, and bind it to socket fd
	laddr.sin_family = AF_INET;
	laddr.sin_port = htons(atoi(SERVERPORT));
	// laddr.sin_addr.s_addr = inet_addr(INADDR_ANY);
	//	inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);
	laddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sd, &laddr, sizeof(laddr)) < 0)
	{
		perror("bind()");
		exit(1);
	}

	// 3rd. switch the socket fd to passive listening mode
	if ( listen(sd, BACKLOG) < 0 )
	{
		perror("listen()");
		exit(1);
	}

	// caller must initialize it to contain the size (in bytes) of the raddr
	raddr_len = sizeof(raddr);
	while (1)
	{
		int newsd = 0;
		// 4th. Wait for accepting a client connection.
		newsd = accept(sd, (void*)&raddr, &raddr_len);
		if (newsd < 0)
		{
			if (errno == EINTR)
				continue;
			perror("accept()");
			exit(1);
		}
		// 5th. Do server. Sending or receiving data.
		serverjob(newsd, &raddr);
		inet_ntop(AF_INET, &raddr.sin_addr, ipstr,  raddr_len);
		printf("Connection from: %s:%d\n", ipstr, ntohs(raddr.sin_port));
		// 6th. Close the newly allocated socket fd.
		close(newsd);
	}
	close(sd);
	return 0;
}

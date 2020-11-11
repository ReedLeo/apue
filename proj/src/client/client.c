#include "client.h"
#include <proto.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <net/if.h>

/**
 *  -M	--mgroup	specify multicast group
 *  -P	--port		specify receive port
 *  -p	--player	specify decoder
 *  -H	--help		display help
 *  implement by getopt_long
 */
struct client_conf_st client_conf = {
	.rcvport	= DEFAULT_RECVPORT,
	.mgroup 	= DEFAULT_MGROUP,
	.player		= DEFAULT_PLAYERCMD
};

int main(int argc, char** argv)
{
	static struct option argarr[] = {
		{"port", required_argument, NULL, 'p'},
		{"mgroup", required_argument, NULL, 'M'},
		{"player", required_argument, NULL, 'p'},
		{"help", no_argument, NULL, 'H'},
		{NULL, 0, NULL, 0}
	};
	
	int c;
	int sd;
	struct ip_mreqn mreq = {0};
	struct sockaddr_in laddr = {0};

	/**
	 * Initialization:
	 * 	Level:(value from)	
	 * 		default value,
	 * 		configuration files,
	 * 		environment value,
	 *		command line parameter
	 */ 		
	while (1)
	{
		int index = 0;
		c = getopt_long(argc, argv, "P:M:p:H", argarr, &index);
		if (c < 0)
			break;
		switch (c)
		{
		case 'P':
			client_conf.rcvport = optarg; 	
			break;

		case 'M':
			client_conf.mgroup = optarg;
			break;
		case 'p':
			client_conf.player = optarg;
			break;

		case 'H':
			printf("Usage:...\n");
			exit(0);
			break;

		default:
			abort();
			break;
		}
	}


	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	// set client to join the multicast group.
	inet_pton(AF_INET, client_conf.mgroup, &mreq.imr_multiaddr);
	inet_pton(AF_INET, "0.0.0.0", &mreq.imr_address);
	mreq.imr_ifindex = if_nametoindex("ens33");

	if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) )
	{
		perror("setsockopt()");
		exit(EXIT_FAILURE);
	}

	laddr.sin_family = AF_INET;
	laddr.sin_port = ntohs(atoi(client_conf.rcvport));
	inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);

	if (bind(sd, (void*)&laddr, sizeof(laddr)) < 0)
	{
		perror("bind()");
		exit(EXIT_FAILURE);
	}	
	
	pipe();

	fork();

	// child: call decoder
	// parent: receive packets, pipe to child.

	exit(0);
}

#define _GNU_SOURCE

#include "client.h"
#include <proto.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
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
	.rcvport		= DEFAULT_PORT,
	.mgroup 		= DEFAULT_MGROUP,
	.player_cmd		= DEFAULT_PLAYERCMD
};

void print_help()
{
	fprintf(stderr, "Usage:\n\t-M	--mgroup	specify multicast group\n\t-P	--port		specify receive port\n\t-p	--player	specify decoder\n\t-H	--help		display help\n");
}

ssize_t writen(int fd, char* buf, size_t count)
{
	ssize_t pos = 0;
	ssize_t len = 0;
	while (count > 0)
	{
		len = write(fd, buf + pos, count);
		if (len < 0)
		{
			if (EINTR == errno)
				continue;
			perror("writen");
			exit(EXIT_FAILURE);
		}
		pos += len;
		count -= len;
	}
	return pos;
}

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
	int pd[2] = {0};
	pid_t pid;
	struct ip_mreqn mreq = {0};
	struct sockaddr_in svaddr = {0};
	struct sockaddr_in cl_addr = {0};
	socklen_t svaddr_len = 0;
	socklen_t cl_addr_len = 0;
	ssize_t recv_len = 0;

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
			client_conf.player_cmd = optarg;
			break;

		case 'H':
			print_help();
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
	mreq.imr_ifindex = if_nametoindex("eth0");

	if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) )
	{
		perror("setsockopt()");
		exit(EXIT_FAILURE);
	}

	svaddr.sin_family = AF_INET;
	svaddr.sin_port = ntohs(atoi(client_conf.rcvport));
	inet_pton(AF_INET, "0.0.0.0", &svaddr.sin_addr);

	if (bind(sd, (void*)&svaddr, sizeof(svaddr)) < 0)
	{
		perror("bind()");
		exit(EXIT_FAILURE);
	}	
	
	if (pipe(pd) < 0)
	{
		perror("pipe()");
		exit(EXIT_FAILURE);
	}

	pid = fork();
	if (pid == 0)
	{
		// child: call media player
		close(sd);	// child dose not need socket.
		close(pd[1]); // child just read form pipe, dose not write.
		dup2(pd[0], 0); // redirect pipe's read end to stdin.
		if (pd[0] > 0)
		{
			close(pd[0]);
		}
		execl("/bin/sh", "sh", "-c", client_conf.player_cmd, NULL);
		perror("execute player failed.");
		exit(EXIT_FAILURE);
	}
	else if (pid < 0)
	{
		perror("fork()");
		exit(EXIT_FAILURE);
	}
	// parent: receive packets, pipe to child.
	close(pd[0]);  // no read.

	// receive program list
	struct msg_list_st* p_msg_list = malloc(MSG_LIST_MAX);
	if (NULL == p_msg_list)
	{
		perror("malloc msg_list");
		exit(EXIT_FAILURE);
	}

	// must set!!
	svaddr_len = sizeof(svaddr);
	while (1)
	{
		recv_len = recvfrom(sd, p_msg_list, MSG_LIST_MAX, 0, (void*)&svaddr, &svaddr_len);
		if (recv_len < sizeof(*p_msg_list))
		{
			fprintf(stderr, "message too small.\n");
			continue;
		}
		if (p_msg_list->chnid != LISTCHNID)
		{
			fprintf(stderr, "chnid is not match.\n");
			continue;
		}
		break;
	}

	// print program list and select channel
	struct msg_listentry_st *p_ent = NULL;
	for (p_ent = p_msg_list->list;
	 	(char*)p_ent < (char*)p_msg_list + recv_len;
	 	p_ent = (void*)((char*)p_ent + ntohs(p_ent->len)) 
	 )
	{
		printf("channel[%d]:%s\n", p_ent->chnid, p_ent->desc);
	}
	chnid_t chosen_id = 0;
	printf("input the number of channel you want to listen: ");
	if (1 != scanf("%d", &chosen_id) )
	{
		perror("select channel failed.");
		free(p_msg_list);
		exit(EXIT_FAILURE);
	}

	free(p_msg_list);

	// receive channel packets and dispatch to the child.
	struct msg_channel_st * p_msg_channel = malloc(MSG_CHANNEL_MAX);
	if (NULL == p_msg_channel)
	{
		perror("malloc for msg_channel_st");
		exit(EXIT_FAILURE);
	}

	cl_addr_len = sizeof(cl_addr);
	while (1)
	{
		recv_len = recvfrom(sd, p_msg_channel, MSG_CHANNEL_MAX, 0, (void*)&cl_addr, &cl_addr_len);

		if (recv_len < sizeof(struct msg_channel_st))
		{
			fprintf(stderr, "Ignore: message too small.\n");
			continue;
		}

		if (cl_addr.sin_addr.s_addr != svaddr.sin_addr.s_addr
		|| cl_addr.sin_port != svaddr.sin_port )
		{
			fprintf(stderr, "Ignore: address not match.\n");
			continue;
		}

		if (p_msg_channel->chnid == chosen_id )
		{
			fprintf(stdout, "accepted msg: %d received.\n", p_msg_channel->chnid);
			writen(pd[1], p_msg_channel->data, recv_len - sizeof(p_msg_channel->chnid));
		}
	}

	free(p_msg_channel);
	close(sd);
	exit(0);
}

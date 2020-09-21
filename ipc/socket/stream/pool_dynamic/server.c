#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <signal.h>

#include "proto.h"

#define MIN_IDLE_PROC 5
#define MAX_IDLE_PROC 10
#define MAX_POOL_SIZE 20

#define SIG_USR_NOTIFY SIGUSR2

#define ST_IDLE 	1
#define ST_RUNNING 	2
#define ST_UNUSED 	4

struct server_st
{
	pid_t pid;
	int state;
	// int reuse; // apache server have usage count.
};

static struct server_st* server_pool;

static void init_proc_pool(int sd)
{
	server_pool = mmap(NULL, MAX_POOL_SIZE * sizeof(struct server_st), PORT_READ | PORT_WRITE, MAP_ANANYMOUS | MAP_SHARED, -1, 0);
	if (server_pool == NULL)
	{
		perror("mmap()");
		exit(1);
	}

	for (int i = 0; i < MAX_POOL_SIZE; ++i)
	{
		server_pool[i].pid = -1;
		server_pool[i].state = ST_UNUSED;
	}
	gs_idle_count = 0;
	gs_proc_count = 0;

	for (int i = 0; i < MIN_IDLE_PROC; ++i)
	{
		pid_t pid = fork();
		if (pid < 0)
		{
			perror("fork()");
			exit(1);
		}
		else if (pid == 0)
		{
			do_server_job(sd);
			exit(0);
		}
		else
		{
			server_pool[i].pid = pid;
			server_pool[i].state = ST_RUNNING;
		}
	}
}

static void destory()
{
	munmap(server_pool, MAX_POOL_SIZE * sizeof(struct server_st));
}

static void scan_pool()
{
	int proc_cnt = 0;
	int idle_cnt = 0;
	for (int i = 0; i < MAX_POOL_SIZE; ++i)
	{
		if (server_pool[i].pid > 0 && server_pool[i].state == ST_RUNNING)
		{
			++proc_cnt;
		}
		else if (server_pool[i].pid < 0 && server_pool[i].state == ST_IDLE)
		{
			++idle_cnt;
		}
		else
		{
			fprintf(stderr, "Unknown state\n");
			abort();
		}
	}
	gs_proc_count = proc_cnt;
	gs_idle_count = idle_cnt;
}

static int add_1_proc(int sd)
{
	int free_idx = -1;
	if (sd < 0)
		return -1;
	for (int i = 0; i < MAX_POOL_SIZE; ++i)
	{
		if (-1 == server_pool[i].pid && ST_UNUSED == server_pool[i].state)
		{
			free_idx = i;
			break;
		}
	}
	
	pid_t pid = fork();
	if (pid < 0)
	{
		perror("fork()");
		exit(1);
	}
	else if (pid == 0)
	{
		do_server_job(sd);
		exit(1);
	}
	else 
	{
		server_pool[free_idx].pid = pid;
		server_pool[free_idx].state = ST_RUNNING;
	}
	return free_idx;	
}

static void remove_1_idle()
{
	for (int i = 0; i < MAX_POOL_SIZE; ++i)
	{
		if (server_pool[i].pid > 0 && server_pool[i].state == ST_IDEL)
		{
			kill(server_pool[i].pid, SIGTERM);
			server_pool[i].pid = -1;
			server_pool[i].state = ST_UNUSED;
		}
	}
}

int main(int argc, char** argv)
{
	int sd = -1;
	struct sockaddr_in laddr = {0};
	// 1st. create socket
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0) 
	{
		perror("socket()");
		exit(1);
	}
	
	// 2nd. bind socket
	laddr.sin_family = AF_INET;
	laddr.sin_port = htons(atoi(SERVERPORT));
	laddr.sin_addr.s_addr = inet_addr(INADDR_ANY);
	if (bind(sd, &laddr, sizeof(laddr)) < 0)
	{
		perror("bind()");
		exit(1);
	}
	// 3rd. set reuser socket
	int val = 1;
	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0)
	{
		perror("setsockopt()");
		exit(1);
	}

	// 4th. listen to socket, turn to listening state.
	if (listen(sd, 200) < 0)
	{
		perror("listen()");
		exit(1);
	}

	// 5th. create proc pools, do accept and response.
	init_proc_pool(sd);

	while (1)
	{
		// droved by SIG_NOTIFY		
		if (sigsuspend(&wait_notify_set) < 0)
		{
			perror("sigsuspend()");
			exit(1);
		}
	
		scanf_pool();

		if (gs_idle_count + gs_proc_count <= MAX_POOL_SIZE)
		{
			if (gs_idle_count >= MAX_IDLE_PROC)
			{
				// Too much idle sbuprocesses.
				// Shrink the pool to its half size.
				int decreased_by_cnt = gs_idle_count / 2;
				for (int i = 0; i < decreased_by_cnt; ++i)
				{
					remove_1_idle();
					--gs_idle_count;
				}
			}
			else if (gs_idle_count <= MIN_IDLE_PROC)
			{
				add_1_proc(sd);
				++gs_idle_count;	
			}	
		}
		else
		{
			perror("Exceeded max pool size");
			abort();
		}
	}
	return 0;
}

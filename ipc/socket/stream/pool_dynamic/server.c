#include <signal.h>
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
#include <sys/mman.h>
#include <time.h>
#include <errno.h>

#include "proto.h"

#define MIN_IDLE_PROC 5
#define MAX_IDLE_PROC 10
#define MAX_POOL_SIZE 20

#define SIG_USR_NOTIFY SIGUSR2

#define ST_IDLE 	1
#define ST_BUSY 	2

#define MAX_LEN_SIZE 128

static int gs_idle_count;
static int gs_busy_count;

struct server_st
{
	pid_t pid;
	int state;
	// int reuse; // apache server have usage count.
};

static struct server_st* server_pool;

static void usr2_handler(int signum)
{
	return;
}

static void do_server_job(int sd, int slot)
{
	int newsd;
	struct sockaddr_in raddr;
	socklen_t	raddr_len;
	time_t stamp;
	char linebuf[MAX_LEN_SIZE] = {0};
	int byte2send;
	int byte_has_sent;

	if (slot < 0 || slot >= MAX_POOL_SIZE)
	{
		fprintf(stderr, "Slot out of range\n");
		return;
	}
	
	pid_t ppid = getppid();
	while (1)
	{
		if (server_pool[slot].pid < 0)
			break;
		// Notify parent its state changed.
		server_pool[slot].state = ST_IDLE;
		kill(ppid, SIG_USR_NOTIFY);

		// accept automatically blocked if there are no connection arrive.
		raddr_len = sizeof(raddr);
		newsd = accept(sd, (void*)&raddr, &raddr_len);
		if (newsd < 0)
		{
			perror("accept()");
			break;
		}

		// Notify parent its state changed.
		server_pool[slot].state = ST_BUSY;
		kill(ppid, SIG_USR_NOTIFY);

		stamp = time(NULL);
		byte2send = snprintf(linebuf, MAX_LEN_SIZE, FMT_STAMP, (long long)stamp);
		send(newsd, linebuf,byte2send, 0);
		
		sleep(5);
		close(newsd);
	}
	close(sd);
}

static void init_proc_pool(int sd)
{
	server_pool = mmap(NULL,MAX_POOL_SIZE * sizeof(struct server_st), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	if (server_pool == NULL)
	{
		perror("mmap()");
		exit(1);
	}

	for (int i = 0; i < MAX_POOL_SIZE; ++i)
	{
		server_pool[i].pid = -1;
	}
	gs_idle_count = 0;
	gs_busy_count = 0;

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
			do_server_job(sd, i);
			exit(0);
		}
		else
		{
			server_pool[i].pid = pid;
			server_pool[i].state = ST_IDLE;
		}
	}
}

static void destory()
{
	munmap(server_pool, MAX_POOL_SIZE * sizeof(struct server_st));
}

static void scan_pool()
{
	int busy_cnt = 0;
	int idle_cnt = 0;
	for (int i = 0; i < MAX_POOL_SIZE; ++i)
	{
		if (server_pool[i].pid > 0)
		{
			if (server_pool[i].state == ST_BUSY)
				++busy_cnt;
			else if (server_pool[i].state == ST_IDLE)
				++idle_cnt;
			else
			{
				fprintf(stderr, "Unknown State.\n");
				abort();
			}
		}
	}
	gs_busy_count = busy_cnt;
	gs_idle_count = idle_cnt;
}

static int add_1_proc(int sd)
{
	int free_idx = -1;
	if (sd < 0)
		return -1;
	for (int i = 0; i < MAX_POOL_SIZE; ++i)
	{
		if (-1 == server_pool[i].pid) 
		{
			free_idx = i;
			break;
		}
	}
	if (free_idx < 0)
		return -1;

	pid_t pid = fork();
	if (pid < 0)
	{
		perror("fork()");
		exit(1);
	}
	else if (pid == 0)
	{
		do_server_job(sd, free_idx);
		exit(0);
	}
	else 
	{
		server_pool[free_idx].pid = pid;
		server_pool[free_idx].state = ST_IDLE;
	}
	return free_idx;	
}

static void remove_1_idle()
{
	for (int i = 0; i < MAX_POOL_SIZE; ++i)
	{
		if (server_pool[i].pid > 0 && server_pool[i].state == ST_IDLE)
		{
			kill(server_pool[i].pid, SIGTERM);
			server_pool[i].pid = -1;
			break;
		}
	}
}

static void display_pool()
{
	for (int i = 0; i < MAX_POOL_SIZE; ++i)
	{
		if (server_pool[i].pid < 0)
			putchar('-');
		else if (server_pool[i].state == ST_IDLE)
			putchar('o');
		else if (server_pool[i].state == ST_BUSY)
			putchar('x');
		else
			putchar('!');
	}
	puts("");
}

int main(int argc, char** argv)
{
	int sd = -1;
	struct sockaddr_in laddr;
	sigset_t nset, oset;
	struct sigaction sa, osa;

	// 1st. create socket
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0) 
	{
		perror("socket()");
		exit(1);
	}
	
	// 2nd. set reuser socket
	int val = 1;
	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0)
	{
		perror("setsockopt()");
		exit(1);
	}

	// 3rd. bind socket
	laddr.sin_family = AF_INET;
	laddr.sin_port = htons(atoi(SERVERPORT));
	laddr.sin_addr.s_addr = inet_addr("0.0.0.0");
	if (bind(sd, &laddr, sizeof(laddr)) < 0)
	{
		perror("bind()");
		exit(1);
	}
	// 4th. listen to socket, turn to listening state.
	if (listen(sd, 200) < 0)
	{
		perror("listen()");
		exit(1);
	}

	sigemptyset(&oset);
	
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = SA_NOCLDWAIT;
	sigaction(SIGCHLD, &sa, &osa);
	
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = usr2_handler;
	sigaction(SIG_USR_NOTIFY, &sa, &osa);

	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIG_USR_NOTIFY);
	sigprocmask(SIG_BLOCK, &sa.sa_mask, &oset);


	// 5th. create proc pools, do accept and response.
	init_proc_pool(sd);

	while (1)
	{
		// droved by SIG_NOTIFY		
		// sigsuspend() always return -1, normally errno == EINTR
		if (sigsuspend(&oset) != -1 || errno != EINTR)
		{
			perror("sigsuspend()");
			exit(1);
		}
	
		// upadte gs_idle_cnt and gs_busy_cnt
		scan_pool();
		
		if (gs_idle_count + gs_busy_count <= MAX_POOL_SIZE)
		{
			if (gs_idle_count >= MAX_IDLE_PROC)
			{
				// Too much idle sbuprocesses.
				// Shrink the pool to its half size.
				int decreased_by_cnt = gs_idle_count / 2;
				for (int i = 0; i < decreased_by_cnt; ++i)
				{
					remove_1_idle();
				}
			}
			else if (gs_idle_count < MIN_IDLE_PROC)
			{
				add_1_proc(sd);
			}	
		}
		else
		{
			perror("Exceeded max pool size");
			abort();
		}

		// Test demon
		display_pool();
	}

	sigprocmask(SIG_SETMASK, &oset, NULL);
	destory();
	return 0;
}

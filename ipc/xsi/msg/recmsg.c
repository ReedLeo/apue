#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>

#include "proto.h"

static int mq = 0 ;

static void destroy(void)
{
	if (mq >= 0)
		msgctl(mq, IPC_RMID, NULL);
}

static void sig_handle(int signum)
{
	destroy();
	exit(signum);
}
typedef void (*sighandler_t)(int);

static void init()
{
	atexit(destroy);
	signal(SIGTERM, sig_handle);
	signal(SIGINT, sig_handle);
	signal(SIGQUIT, sig_handle);
}

int main(int argc, char** argv)
{
	key_t  key;
	struct msg_st msgbuf = {0};

	init();

	key = ftok(KEYPATH, KEYPROJ);
	if (key == -1)
	{
		perror("ftok()");
		exit(1);
	}

	mq = msgget(key, IPC_CREAT | 0600);
	if (mq < 0)
	{
		perror("msgget()");
		exit(1);
	}
	
	while (1)
	{
		if (msgrcv(mq, &msgbuf, sizeof(msgbuf) - sizeof(msgbuf.mtype), 0, 0) < 0)
		{
			perror("msgrcv()");
			exit(1);
		}
		printf("%s\nMath=%d\nChinese=%d\n", msgbuf.name, msgbuf.math, msgbuf.chinese);
	}	
	
	
	return 0;
}

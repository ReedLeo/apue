#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

#include "proto.h"

int main(int argc, char** argv)
{
	key_t key;
	int mqid = 0;
	struct msg_st sndbuf = {0};

	key = ftok(KEYPATH, KEYPROJ);
	if (key < 0)
	{
		perror("ftok()");
		exit(1);
	}

    mqid = msgget(key, 0);
	if (mqid < 0)
	{
		perror("msgget()");
		exit(1);
	}
	
	sndbuf.mtype = 1;
	strcpy(sndbuf.name, "Leo");
	sndbuf.math = 98;
	sndbuf.chinese = 90;

	if (msgsnd(mqid, &sndbuf, sizeof(sndbuf) - sizeof(sndbuf.mtype), 0) < 0)
	{
		perror("msgsnd()");
		exit(1);
	}

	//	It's receiver's responsiblity to remove
	//	message queue.
	//	msgctl();
	return 0;
}

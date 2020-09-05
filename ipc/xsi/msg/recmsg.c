#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "proto.h"

int main(int argc, char** argv)
{
	key_t  key;
	
	key = ftok(KEYPATH, KEYPROJ);
	if (key == -1)
	{
		perror("ftok()")
		exit(1);
	}

	msgget(key, );

	msgrcv();

	msgctl();
	
	return 0;
}

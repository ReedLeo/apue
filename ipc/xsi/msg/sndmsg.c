#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "proto.h"

int main(int argc, char** argv)
{
	ftok();

	msgget();

	msgsnd();

	msgctl();
	return 0;
}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char** argv)
{
	int pfd[2] = {0};
	char buf[100] = {0};
	pid_t pid;
			
	if (argc < 2)
	{
		fprintf(stderr, "Too few arguments\n");
		exit(1);
	}

	if (pipe(pfd) < 0)
	{
		perror("pipe()");
		exit(1);
	}
	
	pid = fork();
	if (pid < 0)
	{
		perror("fork()");
		exit(1);
	}
	else if (pid > 0)
	{
		close(pfd[0]);
		write(pfd[1], argv[1], strlen(argv[1]));
		wait(NULL);
	}
	else
	{
		close(pfd[1]);
		read(pfd[0], buf, sizeof buf);
		printf("Child process: Received \"%s\" parent process.", buf);
		exit(0);
	}
	return 0;
}

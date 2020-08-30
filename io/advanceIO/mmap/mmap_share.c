#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

#define MMAP_SIZE 4096

int main(int argc, char** argv)
{
	char* str = NULL;
	int pid = -1;

	str = mmap(NULL, MMAP_SIZE, PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	if (str == NULL)
	{
		perror("mmap()");
		exit(EXIT_FAILURE);
	}
	
	pid = fork();
	if (pid < 0)
	{
		perror("fork()");
		exit(EXIT_FAILURE);
	}	
	if (pid)
	{
		strcpy(str, "Hello MMAP");
		wait(NULL);
		munmap(str, MMAP_SIZE);
	}
	else
	{
		printf("What we get in mmap buffer is: %s\n", str);
		munmap(str, MMAP_SIZE);
		exit(EXIT_SUCCESS);
	}
	return 0;
}

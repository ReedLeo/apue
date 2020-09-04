#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define PLAYER_NAME "mpg123"

int main(int argc, char** argv)
{
	int pfd[2] = {0};
	int mp3_fd;
	int byte_written;
	ssize_t pos;
	long file_size;
	char* buf = NULL;
	pid_t pid;
	FILE* fp = NULL;
			
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
		
		fp = fopen(argv[1], "r");
		if (fp == NULL)
		{
			perror("fopen()");
			exit(1);
		}
		
		if (fseek(fp, 0, SEEK_END) < 0)
		{
			perror("fseek()");
			exit(1);
		}

		file_size = ftell(fp);
		if (file_size < 0)
		{
			perror("ftell()");
			exit(1);
		}

		mp3_fd = fileno(fp);
		if (mp3_fd < 0)
		{
			perror("fileno()");
			exit(1);
		}

		buf = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, mp3_fd, 0);
		if (buf == NULL)
		{
			perror("mmap()");
			exit(1);
		}
		
		pos = byte_written = 0;
		while (pos < file_size)
		{
			byte_written = write(pfd[1], buf + pos, file_size - pos);
			if (byte_written < 0)
			{
				if (errno == EINTR)
					continue;
				perror("write()");
				exit(1);
			}
			pos += byte_written;
		}

		wait(NULL);
		close(pfd[1]);
		munmap(buf, file_size);
	}
	else
	{
		close(pfd[1]);
		dup2(pfd[0], 0);
		execlp(PLAYER_NAME, PLAYER_NAME, "-", NULL);
		perror("execlp()");
		exit(1);
	}
	return 0;
}

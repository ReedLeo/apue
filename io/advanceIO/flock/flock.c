#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <limits.h>

#define MAX_THR_NUM 20
#define FNAME "/tmp/out"

static int fd = -1;

static void* thr_proc(void* arg)
{
	char buf[100] = {0};
	long cnt = 0;

	if (lockf(fd, F_LOCK, 0) < 0)
	{
		perror("lockf()");
		goto err_exit;
	}
	
	if (lseek(fd, 0, SEEK_SET) < 0)
	{
		perror("lseek()");
		goto err_exit;
	}
	
	while (read(fd, buf, sizeof(buf)) < 0)
	{
		if (EINTR == errno)
			continue;
		perror("read()");
		goto err_exit;
	}
	
	cnt = strtol(buf, NULL, 0) + 1;
	if (cnt == LONG_MIN || cnt == LONG_MAX)
	{
		perror("strtol()");
		goto err_exit;
	}
	
	sprintf(buf, "%ld", cnt);

	if (lseek(fd, 0, SEEK_SET) < 0)
	{
		perror("lseek()");
		goto err_exit;
	}
		
	while (write(fd, buf, strlen(buf)) < 0 )
	{
		if (EINTR == errno)
			continue;
		perror("write()");
		goto err_exit;
	}
	
err_exit:
	if (lockf(fd, F_ULOCK, 0) < 0)
	{
		perror("lockf()");
	}
	pthread_exit(NULL);
}

int main(int argc, char** argv)
{
	pthread_t tids[MAX_THR_NUM];
	int ret = 0;

	fd = open(FNAME, O_RDWR|O_CREAT, 0666);
	if (fd < 0)
	{
		perror("open");
		exit(EXIT_FAILURE);
	}
	
	// len == 0 means lock to the infinity end.
	if (lockf(fd, F_LOCK, 0) < 0)
	{
		perror("lockf()");
		exit(EXIT_FAILURE);
	}
	
	write(fd, "1", 2);

	if (lockf(fd, F_ULOCK, 0) < 0)
	{
		perror("lockf()");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < MAX_THR_NUM; ++i)
	{
		ret = pthread_create(tids + i, NULL, thr_proc, NULL);
		if (ret)
		{
			fprintf(stderr, "pthread_create failed with %s\n", strerror(ret));
			exit(EXIT_FAILURE);
		}
	}

	for (int i = 0; i < MAX_THR_NUM; ++i)
	{
		if ((ret = pthread_join(tids[i], NULL)))
		{
			fprintf(stderr, "pthread_join() failed with %s\n", strerror(ret));
			exit(EXIT_FAILURE);
		}
	}
	
	close(fd);
	
	return 0;
}



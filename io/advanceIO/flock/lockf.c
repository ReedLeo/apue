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
static FILE* fp;

static void* thr_proc(void* arg)
{
	long cnt = 0;

	if (lockf(fd, F_LOCK, 0) < 0)
	{
		perror("lockf()");
		goto err_exit;
	}
	
	fseek(fp, 0, SEEK_SET);
	fscanf(fp, "%ld", &cnt);
	++cnt;
	
	fseek(fp, 0, SEEK_SET);
	fprintf(fp, "%ld", cnt);
	fflush(fp);

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

	fp = fopen(FNAME, "w+");
	fd = fileno(fp);

	// len == 0 means lock to the infinity end.
	if (lockf(fd, F_LOCK, 0) < 0)
	{
		perror("lockf()");
		exit(EXIT_FAILURE);
	}
	
	fprintf(fp, "1");
	fflush(fp);

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
	
	fclose(fp);
	
	return 0;
}



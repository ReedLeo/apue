#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

#define THR_MAX 10

static int gs_cnt;
static pthread_mutex_t gs_mut = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gs_cond = PTHREAD_COND_INITIALIZER;

static void* thr_proc(void* arg)
{
	pid_t tid = gettid();
	printf("thread[%d] stats!\n", tid);
	pthread_mutex_lock(&gs_mut);
	while (gs_cnt == 0)
		pthread_cond_wait(&gs_cond, &gs_mut);
	++gs_cnt;
	sleep(1);
	pthread_mutex_unlock(&gs_mut);
	printf("thread[%d] exits!\n", tid);
	pthread_exit(NULL);
}

int main(int argc, char** argv)
{
	int res = 0;
	pthread_t tids[THR_MAX] = {0};
	
	pthread_mutex_lock(&gs_mut);

	for (int i = 0; i < THR_MAX; ++i)
	{
		res = pthread_create(tids + i, NULL, thr_proc, NULL);
		if (res)
		{
			fprintf(stderr, "pthread_create() failed with %s\n", strerror(res));
			exit(EXIT_FAILURE);
		}
	}

	getchar();
	gs_cnt = 1;
	pthread_cond_broadcast(&gs_cond);
	pthread_mutex_unlock(&gs_mut);

	for (int i = 0; i < THR_MAX; ++i)
	{
		res = pthread_join(tids[i], NULL);
		if (res)
		{
			fprintf(stderr, "pthread_join() failed with %s\n", strerror(res));
			exit(EXIT_FAILURE);
		}
	}
	printf("Now, we get count = %d\n", gs_cnt);
	puts("Bye~");

	return 0;	
}

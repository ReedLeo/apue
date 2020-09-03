#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#include "mypipe.h"

#define CHECK(condition) \
	do {\
		if (!(condition)) {\
			printf("ERROR: %s (@%d): failed condtion \"%s\"\n", __func__, __LINE__, #condition);\	
			exit(1);\
		}\
	}while (0)
	
int simpleTest(void);

int main(int argc, char** argv)
{
	if (simpleTest())
	{
		puts("**********************");
		puts("*       Failed       *");
		puts("**********************");
	}
	else
	{
		puts("**********************");
		puts("*       Passed       *");
		puts("**********************");
	}
	return 0;
}


struct reg_info_st
{
	mypipe_t* mp;
	int opmap;
};

static void* thr_register(void* reginfo)
{
	int res = 0;
	struct reg_info_st* info = reginfo;
	assert(info);
	
	res = mypipe_register(info->mp, info->opmap);
	CHECK(res == 0);

	pthread_exit(NULL);
}

int simpleTest(void)
{	
	mypipe_t* mp = mypipe_init();
	CHECK(mp != NULL);
	
	pthread_t tids[2];
	struct reg_info_st info[2] = {0};

	info[0].mp = mp;
	info[0].opmap = MYPIPE_READ;

	info[1].mp = mp;
	info[1].opmap = MYPIPE_WRITE;

	pthread_create(tids, NULL, thr_register, info);
	pthread_create(tids+1, NULL, thr_register, info + 1);
	
	pthread_join(tids[0], NULL);
	pthread_join(tids[1], NULL);
	
	// test read write
	char sndbuf[] = "Hello mypipe.";
	char recbuf[256] = {0};
	int cnt = 0;
	cnt = mypipe_write(mp, sndbuf, sizeof sndbuf);
	CHECK(cnt == sizeof(sndbuf));
	
	cnt = mypipe_read(mp, recbuf, sizeof(recbuf));
	CHECK(cnt == sizeof(sndbuf));
	CHECK(strcmp(sndbuf, recbuf) == 0);

	int res = 0;
	res = mypipe_unregister(mp, MYPIPE_READ);
	CHECK(res == 0);

	res = mypipe_unregister(mp, MYPIPE_WRITE);
	CHECK(res == 0);
	
	res = mypipe_unregister(NULL, MYPIPE_READ);
	CHECK(res != 0);
	CHECK(errno == EINVAL);

	res = mypipe_unregister(mp, MYPIPE_WRITE << 3);
	CHECK(res != 0);
	CHECK(errno == EINVAL);

	res = mypipe_unregister(mp, MYPIPE_READ | MYPIPE_WRITE);
	CHECK(res != 0);
	CHECK(errno == EAGAIN);

	res = mypipe_destroy(mp);
	CHECK(res == 0);

	res = mypipe_destroy(NULL);
	CHECK(res != 0);
	CHECK(errno == EINVAL);
	
	return 0;
}


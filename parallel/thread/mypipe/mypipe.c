#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "mypipe.h"

struct mypipe_st
{
	int head;
	int tail;
	char data[PIPESIZE];
	int datasize;
	int count_rd;	// number of readers
	int count_wr;	// number of writers
	pthread_mutex_t mut;
	pthread_cond_t cond;
};

static int mypipe_readbyte_unlocked(struct mypipe_st* pobj, char* pdata)
{
	if (pobj == NULL || pdata == NULL)
		return -1;
	*pdata = pobj->data[pobj->head];
	// need determine whether the pip is empty.
	pobj->head++;
	pobj->datasize-- ;
	
	return 0;
}
mypipe_t* mypipe_init(void)
{
	struct mypipe_st* me = NULL;
	int res = 0;

	me = malloc(sizeof *me);
	if (me == NULL)
		return NULL;
	
	me->head = 0;
	me->tail = 0;
	me->datasize = 0;
	if ((res = pthread_mutex_init(&me->mut, NULL)) != 0)
	{
		free(me);
		return NULL;
	}
	if ((res = pthread_cond_init(&me->cond, NULL)) != 0)
	{
		pthread_mutex_destroy(&me->mut);
		free(me);
		return NULL;
	}
	return me;
}

int mypipe_read(mypipe_t* pobj, void* buf, size_t count)
{
	struct mypipe_st* me = pobj;

	if (me == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	// there may be mutilthreads read pipe concurrently.
	pthread_mutex_lock(&me->mut);
	for (int i = 0; i < count; ++i)
	{
		// wait until data is available in pipe or no more writers wait for writting.
		while (me->datasize <= 0 && me->count_wr > 0)
			pthread_cond_wait(&me->cond, &me->mut);
		if (me->datasize <= 0 && me->count_wr <= 0)
			break;
		// read one byte into buf+i from pipe
		mypipe_readbyte_unlocked(me, buf+i);
	}
	pthread_mutex_unlock(&me->mut);
	
	return 0;
}

int mypipe_write(mypipe_t*, const void* buf, size_t)
{
	return 0;
}

int mypipe_destory(mypipe_t*)
{
	return 0;
}



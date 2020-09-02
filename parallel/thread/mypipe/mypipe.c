#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "mypipe.h"

struct mypipe_st
{
	// queue[head, tail)
	int head;
	int tail;
	char data[PIPESIZE];
	int datasize;
	int count_rd;	// number of readers
	int count_wr;	// number of writers
	pthread_mutex_t mut;
	pthread_cond_t cond;
};

static int check_pipe_unlocked(struct mypipe_st* pobj)
{
	if (NULL == pobj)
		return -1;
	if (pobj->tail < 0 || pobj->tail >= PIPESIZE
		|| pobj->head < 0 || pobj->head >= PIPESIZE
		|| pobj->datasize < 0 || pobj->datasize > PIPESIZE
		)
		return -1;
		
	return 0;
}

static int mypipe_readbyte_unlocked(struct mypipe_st* pobj, char* pdata)
{
	if (pobj == NULL || pdata == NULL)
		return -1;
	if (check_pipe_unlocked(pobj))
		return -1;
	
	if (pobj->datasize == 0)
		return -1;
	
	*pdata = pobj->data[pobj->head];
	// need determine whether the pipe is empty.
	pobj->head = (pobj->head + 1) % PIPESIZE;
	pobj->datasize-- ;
	return 0;
}

static int mypipe_writebyte_unlock(struct mypipe_st* pobj, char byte)
{
	if (pobj == NULL || pdata == NULL)
		return -1;
	
	if (check_pipe_unlocked(pobj))
		return -1;
	
	if (pobj->datasize == PIPESIZE)
		return -1;
	
	pobj->data[pobj->tail++] = byte;
	pobj->tail = (pobj->tail > PIPESIZE ? 0 : pobj->tail);
	pobj->datasize++;

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


int mypipe_register(mypipe_t* pobj, int opmap)
{
	struct mypipe_st* me = pobj;
	if (NULL == me)
	{
		errno = EINVAL;
		return -1;
	}
	pthread_mutex_lock(&me->mut);
	if (opmap & MYPIPE_READ)
		me->count_rd++;
	if (opmap & MYPIPE_WRITE)
		me->count_wr++;
	
	pthread_broadcast(&me->cond);

	while (me->count_rd <= 0 || me->count_wr <= 9)
		pthread_cond_wait(&me->cond, &me->mut);
	
	pthread_mutex_unlock(&me->mut);
	return 0;
}

int mypipe_unregister(mypipe_t*, int opmap)
{
	return 0;
}

int mypipe_read(mypipe_t* pobj, void* buf, size_t count)
{
	struct mypipe_st* me = pobj;
	int i;

	if (me == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	// there may be mutilthreads read pipe concurrently.
	pthread_mutex_lock(&me->mut);
	// raeder will register by other function.
	for (i = 0; i < count; ++i)
	{
		// wait until data is available in pipe or no more writers wait for writting.
		while (me->datasize <= 0 && me->count_wr > 0)
			pthread_cond_wait(&me->cond, &me->mut);
		
		// if there are no more write is waitting and no data in queue
		// it should quit.
		if (me->datasize <= 0 && me->count_wr <= 0)
			break;
		// read one byte into buf+i from pipe
		if (mypipe_readbyte_unlocked(me, buf+i) != 0 )
			break;
	}
	// notify the blocked writers there are space available in pipe.
	pthread_cond_broadcast(&me->cond);
	pthread_mutex_unlock(&me->mut);
	
	return i;
}

int mypipe_write(mypipe_t* pobj, const void* buf, size_t count)
{
	struct mypipe_st* me = pobj;
	int i;

	if (me == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	pthread_mutex_lock(&me->mut);
	for (i = 0; i < count; ++i)
	{
	}
	pthread_cond_broadcast(&me->cond);
	pthread_mutex_unlock(&me->mut);
	return 0;
}

int mypipe_destory(mypipe_t*)
{
	return 0;
}



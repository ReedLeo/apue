#ifndef __MYPIPE_H__
#define __MYPIPE_H__

#define PIPESIZE 1024

typedef void mypipe_t;;

mypipe_t* mypipe_init(void);

int mypipe_read(mypipe_t*, void* buf, size_t size);

int mypipe_write(mypipe_t*, const void* buf, size_t);

int mypipe_destory(mypipe_t*);

#endif

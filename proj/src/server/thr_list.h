#ifndef _THR_LIST_H__
#define _THR_LIST_H__

#include "medialib.h"

int thr_list_create(struct mlib_listentry_st* p_list, int list_size);
int thr_list_destory(void);

#endif
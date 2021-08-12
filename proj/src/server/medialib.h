#ifndef _MEDIALIB_H__
#define _MEDIALIB_H__

#include <site_type.h>

struct mlib_listentry_st
{
    chnid_t chnid;
    char* desc;
};

int mlib_getchnlist(struct mlib_listentry_st**, int* );
int mlib_readchn(chnid_t chnid, void* buf, size_t size);

#endif
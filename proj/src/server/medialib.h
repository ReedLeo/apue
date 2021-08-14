#ifndef _MEDIALIB_H__
#define _MEDIALIB_H__

#include <site_type.h>

struct mlib_listentry_st
{
    chnid_t chnid;
    char* desc;
};

int mlib_getchnlist(struct mlib_listentry_st**, int* );
int mlib_freechnlist(void);
ssize_t mlib_readchn(const chnid_t chnid, void* buf, const size_t size);

#endif
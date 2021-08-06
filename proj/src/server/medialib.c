#include "medialib.h"
#include <stdio.h>
#include <stdlib.h>
#include <glob.h>
#include <sys/types.h>

struct channel_context_st
{
    chnid_t chnid;
    char* desc;
    glob_t mp3glob;
    int pos;
    int fd;         // file descriptor of the open media file.
    off_t offset;   // file offset, from where transmit.
    mytbf_t* tbf;   // flow control
};

int mlib_getchnlist(struct mlib_listentry_st** pp_list, int* p_list_size)
{
    // TODO
    return 0;
}
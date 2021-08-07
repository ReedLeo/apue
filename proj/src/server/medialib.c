#include <stdio.h>
#include <stdlib.h>
#include <glob.h>
#include <sys/types.h>
#include <glob.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>

#include <proto.h>
#include "medialib.h"
#include "mytbf.h"
#include "server_conf.h"

#define PATHSIZE    1024

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

static struct channel_context_st gs_channels[MAXCHNID + 1];

static
int path2entry(const char* path)
{
    return 0;
}

int mlib_getchnlist(struct mlib_listentry_st** pp_list, int* p_list_size)
{
    int valid_channel_num = 0;
    char path[PATHSIZE] = {0};
    struct mlib_listentry_st* p_list = NULL;
    glob_t glob_res;

    for (int i = 0; i < MAXCHNID + 1; ++i)
    {
        // initialize the channel id to be invalid.
        gs_channels[i].chnid = -1;
    }

    snprintf(path, PATHSIZE, "%s/*", g_svconf.media_dir);
    if (glob(path, 0, NULL, &glob_res))
    {
        // path analysis failed.
        globfree(&glob_res);
        return -1;
    }

    // there are gl_pathc directories or files, but not all of them are valid
    // media channel.
    p_list = malloc(glob_res.gl_pathc * sizeof(struct mlib_listentry_st));
    if (p_list == NULL)
    {
        syslog(LOG_ERR, "malloc() failed.(%s)", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // count the valid media channel number.
    for (unsigned i = 0; i < glob_res.gl_pathc; ++i) 
    {
        path2entry(glob_res.gl_pathv[i]);
        ++valid_channel_num;
    }

    *pp_list = p_list;
    *p_list_size = valid_channel_num;
    globfree(&glob_res);
    return 0;
}
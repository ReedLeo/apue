#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <glob.h>
#include <sys/types.h>
#include <glob.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <proto.h>
#include "medialib.h"
#include "mytbf.h"
#include "server_conf.h"

#define PATHSIZE    1024
#define DESC_MAX_LEN    1024
#define DESC_NAME   "/desc.txt"
#define MP3_PATTERN "/*.mp3"
// mp3's bitrate: 128kbit/s
#define MP3_BITRATE (128 * 1024)

struct channel_context_st
{
    chnid_t chnid;
    char* desc;     // points to current channel's description.
    glob_t mp3glob; // all the mp3 files' path information in this channel.
    int pos;        // mp3glob.pathv[pos] points to current mp3 file's path.
    int fd;         // file descriptor of the open media file.
    off_t offset;   // file offset, from where to transfer.
    mytbf_t* tbf;   // flow control
};

static struct channel_context_st gs_channels[MAXCHNID + 1];
static struct mlib_listentry_st* gs_p_list;
static int gs_list_size;

static
int open_next(chnid_t chnid)
{
    int cur_pos = 0;
    int fd = -1;

    for (int i = 0; i < gs_channels[chnid].mp3glob.gl_pathc; ++i) {
        cur_pos = gs_channels[chnid].pos + 1;
        close(gs_channels[chnid].fd);
        gs_channels[chnid].fd = -1;
        if (cur_pos >= gs_channels->mp3glob.gl_pathc) {
            // go back and try again.
            cur_pos = 0;
        }
        
        fd = open(gs_channels[chnid].mp3glob.gl_pathv[cur_pos], O_RDONLY);
        if (fd < 0) {
            syslog(LOG_WARNING, "In %s(): open(%s, O_RDONLY) failed. (%s)", __func__, gs_channels[chnid].mp3glob.gl_pathv[cur_pos], strerror(errno));
            continue;
        }
        // fd >= 0, open success, return.
        gs_channels[chnid].fd = fd;
        gs_channels[chnid].offset = 0;
        return 0;
    }
    syslog(LOG_ERR, "There are no mp3 file can be opened in %d channel", chnid);
    return -1;
}

static
int path2entry(const char* path, struct mlib_listentry_st* p_ent, const int cur_pos)
{
    glob_t glob_res = {0};
    char mp3_path[PATHSIZE] = {0};
    char desc_path[PATHSIZE] = {0};
    char desc_content[DESC_MAX_LEN] = {0};
    FILE* fp = NULL;
    int cur_chnid = cur_pos;

    if (NULL == path || NULL == p_ent || cur_chnid < MINCHNID || cur_chnid > MAXCHNID) {
        syslog(LOG_ERR, "In %s(path=%s, p_ent=%p, cur_chnid=%d): passed invalid parameter(s).", __func__, (path ? path : "null"), p_ent, cur_chnid);
        errno = EINVAL;
        return -1;
    }

    strncpy(desc_path, path, PATHSIZE);
    strncat(desc_path, DESC_NAME, PATHSIZE - strlen(desc_path) - 1);
    
    strncpy(mp3_path, path, PATHSIZE);
    strncat(mp3_path, MP3_PATTERN, PATHSIZE - 1 - strlen(mp3_path));

    fp = fopen(desc_path, "r");
    if (NULL == fp) {
        syslog(LOG_ERR, "In %s(): %s is not a valid path to a channel. fopen(%s, 'r') failed. (%s)", __func__, path, desc_path, strerror(errno));
        return -1;
    }

    if (fgets(desc_content, DESC_MAX_LEN, fp) == NULL) {
        syslog(LOG_ERR, "In %s: fgets() failed. cannot read descriptions from %s. (%s)", __func__, desc_path, strerror(errno));
        return -1;
    }

    fclose(fp);
    fp = NULL;
    
    gs_channels[cur_pos].desc = strdup(desc_content);
    if (NULL == gs_channels[cur_pos].desc) {
        syslog(LOG_ERR, "In %s(): strdup(%s) description failed. (%s)", __func__, desc_content, strerror(errno));
        gs_channels[cur_pos].chnid = -1;
        return -1;
    }
    
    gs_channels[cur_pos].offset = 0;  // transfer starts from every file's begining.
    gs_channels[cur_pos].pos = 0;     // start from the 1st mp3.
    gs_channels[cur_pos].chnid = cur_chnid;
    gs_channels[cur_pos].tbf = mytbf_init(MP3_BITRATE / 8, MP3_BITRATE / 8 * 10);
    if (NULL == gs_channels[cur_pos].tbf) {
        syslog(LOG_ERR, "In %s(): mytbf_init(cir=%d, cbs=%d) failed.", __func__, MP3_BITRATE / 8, MP3_BITRATE / 8 * 10);
        free(gs_channels[cur_pos].desc);
        gs_channels[cur_pos].desc = NULL;
        gs_channels[cur_pos].chnid = -1;
        return -1;
    }

    p_ent->chnid = cur_chnid;
    p_ent->desc = strdup(desc_content);
    if (NULL == p_ent->desc) {
        syslog(LOG_ERR, "In %s(): strdups(%s) description failed[2]. (%s)", __func__, desc_content, strerror(errno));
        p_ent->chnid = -1;

        free(gs_channels[cur_pos].desc);
        gs_channels[cur_pos].desc = NULL;
        gs_channels[cur_pos].chnid = -1;
        return -1;
    }
    
    syslog(LOG_DEBUG, "In %s:%d:%s(): now searching under '%s'", __FILE__, __LINE__, __func__, mp3_path);
    if (glob(mp3_path, 0, NULL, &gs_channels[cur_pos].mp3glob)) {
        syslog(LOG_WARNING, "searching mp3 under '%s' failed.", path);
        globfree(&gs_channels[cur_pos].mp3glob);
        free(gs_channels[cur_pos].desc);
        mytbf_destroy(gs_channels[cur_pos].tbf);
        gs_channels[cur_pos].desc = NULL;
        gs_channels[cur_pos].chnid = -1;
        free(p_ent->desc);
        p_ent->desc = NULL;        
        return -1;
    }

    gs_channels[cur_pos].fd = open(gs_channels[cur_pos].mp3glob.gl_pathv[0], O_RDONLY);
    if (gs_channels[cur_pos].fd < 0) {
        syslog(LOG_ERR, "open(%s, O_RDONLY) failed. (%s)", gs_channels[cur_pos].mp3glob.gl_pathv[0], strerror(errno));
        globfree(&gs_channels[cur_pos].mp3glob);
        free(gs_channels[cur_pos].desc);
        mytbf_destroy(gs_channels[cur_pos].tbf);
        gs_channels[cur_pos].desc = NULL;
        gs_channels[cur_pos].chnid = -1;
        free(p_ent->desc);
        p_ent->desc = NULL;
        return -1;
    }

    return 0;
}

int mlib_getchnlist(struct mlib_listentry_st** pp_list, int* p_list_size)
{
    int valid_channel_num = 0;
    char path[PATHSIZE] = {0};
    struct mlib_listentry_st* p_list = NULL;
    struct channel_context_st *p_res = NULL;
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
        syslog(LOG_ERR, "In %s(): glob(path=%s) failed.", __func__, path);
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
        if (path2entry(glob_res.gl_pathv[i], &p_list[valid_channel_num], valid_channel_num+1) < 0)
            continue;
        ++valid_channel_num;
    }

    gs_p_list = *pp_list = p_list;
    gs_list_size = *p_list_size = valid_channel_num;
    
    syslog(LOG_INFO, "In %s(): success! There are %d valid channels at '%s'", __func__, valid_channel_num, g_svconf.media_dir);
    globfree(&glob_res);
    return 0;
}

int mlib_freechnlist(void)
{
    syslog(LOG_DEBUG, "In %s(): there are %d channel list entries to free.", __func__, gs_list_size);
    if (gs_p_list) {
        for (int i = 0; i < gs_list_size; ++i) {
            syslog(LOG_DEBUG, "In %s(): now free the No.%d description.(%s)", __func__, i, gs_p_list[i].desc);
            free(gs_p_list[i].desc);
        }
    }

    for (int i = 0; i < MAXCHNID + 1; ++i) {
        if (gs_channels[i].chnid >=0 && gs_channels[i].chnid <= MAXCHNID) {
            syslog(LOG_DEBUG, "In %s(): now clean the channel[%d]. fd=%d desc='%s'", __func__, gs_channels[i].chnid, gs_channels[i].fd, gs_channels[i].desc);
            gs_channels[i].chnid = -1;
            gs_channels[i].pos = 0;
            gs_channels[i].offset = 0;

            close(gs_channels[i].fd);

            free(gs_channels[i].desc);
            gs_channels[i].desc = NULL;

            globfree(&gs_channels[i].mp3glob);

            mytbf_destroy(gs_channels[i].tbf);
        }
    }
    return 0;
}

/**
 * @param:
 *  chnid: channel id, from which reading mp3 files size bytes to buf.
 *  buf: buffer to store the media data.
 *  size: the number of bytes you want to read.
 * 
 * @return:
 *  >=0: the bytes actually read.
 *  -1: failed.
*/
ssize_t mlib_readchn(const chnid_t chnid, void* buf, const size_t size)
{
    ssize_t bytes_read = 0;
    ssize_t token_fetched = 0;
    if (chnid < 0 || chnid > MAXCHNID || NULL == buf) {
        syslog(LOG_WARNING, "pass to %s() invalid paramter(s).", __func__);
        errno = EINVAL;
        return -1;
    }

    while (1) {
        token_fetched = mytbf_fetch_token(gs_channels[chnid].tbf, size);
        bytes_read = pread(gs_channels[chnid].fd, buf, token_fetched, gs_channels[chnid].offset);
        if (bytes_read < 0) {
            syslog(LOG_WARNING, "In %s(): chnid=%d pread(fd=%d) failed. (%s)", __func__, chnid, gs_channels[chnid].fd, strerror(errno));
            // current opened mp3 file goes wrong, try open the next one.
            if (open_next(chnid) < 0) {
                // none of mp3 are available in channel chnid.
                return -1;
            }
        } else if (0 == bytes_read) {
            syslog(LOG_INFO, "In %s(): chnid=%d %s be sent over.", __func__, chnid, gs_channels[chnid].mp3glob.gl_pathv[gs_channels[chnid].pos]);
            if (open_next(chnid) < 0) {
                // none of mp3 are available in channel chnid.
                return -1;
            }
        } else {
            gs_channels[chnid].pos += bytes_read;
            break;
        }
    }

    mytbf_return_token(gs_channels[chnid].tbf, token_fetched - bytes_read);
    return bytes_read;
}
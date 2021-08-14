#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <errno.h>

#include "thr_channel.h"
#include "server_conf.h"
#include "proto.h"


struct thr_channel_ent_st
{
    chnid_t chnid;  // current channel id.
    pthread_t tid;  // thread id of this channel.
    struct msg_channel_st* p_snd_buf;   // pointer of data to send.
};

struct thr_channel_ent_st gs_thr_channels[CHNNR];

// the next free slot index of thr_channel_ent_st[].
static int gs_next_pos;

static
void* channel_sender_routine(void* p_arg)
{
    ssize_t bytes_to_send = 0;
    ssize_t bytes_read = 0;
    struct thr_channel_ent_st* p_self = p_arg;

    p_self->p_snd_buf = malloc(MSG_CHANNEL_MAX);
    if (NULL == p_self->p_snd_buf) {
        syslog(LOG_ERR, "In %s(): chnid=%d, tid=%lu, malloc() failed.(%s)", __func__, p_self->chnid, gettid(), strerror(errno));
        exit(EXIT_FAILURE);
    }
    p_self->p_snd_buf->chnid = p_self->chnid;
    while (1) {
        bytes_read = mlib_readchn(p_self->chnid, p_self->p_snd_buf->data, MAX_DATA);
        if (bytes_read < 0) {
            syslog(LOG_ERR, "In %s(): chnid=%d, tid=%lu, read data from midea lib failed.", __func__, p_self->chnid, gettid());
            break;
        }
        // totoal size of the real struct msg_channel_st.
        bytes_to_send = bytes_read + sizeof(chnid_t);
        syslog(LOG_INFO, "In %s(): chnid=%d, tid=%lu, try to send %ld bytes.", __func__, p_self->chnid, gettid(), bytes_to_send);
        if (sendto(g_svfd, p_self->p_snd_buf, bytes_to_send, 0, (void*)&g_svaddr, g_svaddr_len) < 0) {
            syslog(LOG_ERR, "In %s(): chnid=%d, tid=%lu, bytes_to_send=%ld sendto() failed. (%s)", __func__, p_self->chnid, gettid(), bytes_to_send, strerror(errno));
            break;
        } else {
            syslog(LOG_INFO, "In %s(): chnid=%d, tid=%lu success send a channel data of %ld byte(s).", __func__, p_self->chnid, gettid(), bytes_to_send);
        }
        // give up the time-slice, let the kernel schedule to other processes.
        sched_yield();
    }
    return NULL;
}

static
void reset_thr_chn_ent(struct thr_channel_ent_st* p_ent)
{
    if (NULL == p_ent)
        return;
    
    memset(p_ent, 0, sizeof(p_ent));
    p_ent->chnid = -1;
}

/**
 * @param:
 *  p_listent: pointer to the list to be sent with valid channel id.
 * 
 * @return:
 *  0: success.
 *  -1: failed and set errno.
*/
int thr_channel_create(struct mlib_listentry_st* p_listent)
{
    int err = 0;
    if (NULL == p_listent || p_listent->chnid < 0 || p_listent->chnid > MAXCHNID) {
        syslog(LOG_ERR, "In %s(p_listent=%p): passed invalid paramter(s).", __func__, p_listent);
        errno = EINVAL;
        return -1;
    }

    if (gs_next_pos >= CHNNR) {
        syslog(LOG_ERR, "In %s(): channel is full.", __func__);
        errno = ENOSPC;
        return -1;
    }
    reset_thr_chn_ent(gs_thr_channels + gs_next_pos);
    gs_thr_channels[gs_next_pos].chnid = p_listent->chnid;
    err = pthread_create(&gs_thr_channels[gs_next_pos].tid,
                        NULL,
                        channel_sender_routine,
                        gs_thr_channels+gs_next_pos
    );
    if (err) {
        syslog(LOG_ERR, "In %s(): pthread_create() failed. (%s)", __func__, strerror(err));
        errno = err;
        return -1;
    }

    ++gs_next_pos;
    return 0;
}

/**
 * @param:
 *  p_listent: points to the channel you want to destroy.
 * 
 * @return:
 *  0: success.
 *  -1: failed and set errno.
*/
int thr_channel_destroy(struct mlib_listentry_st* p_listent)
{
    int err = 0;
    if (NULL == p_listent)
        return 0;
    
    if (p_listent->chnid < MINCHNID || p_listent->chnid > MAXCHNID) {
        errno = EINVAL;
        return -1;
    }

    for (int i = 0; i < CHNNR; ++i) {
        if (gs_thr_channels[i].chnid == p_listent->chnid) {
            err = pthread_cancel(gs_thr_channels[i].tid);
            if (err) {
                syslog(LOG_ERR, "In %s(): chnid=%d pthread_cancel() failed. tid=%ld (%s)", __func__, gs_thr_channels[i].chnid, gs_thr_channels[i].tid, strerror(err));
                errno = err;
                return -1;
            }
            err = pthread_join(gs_thr_channels[i].tid, NULL);
            if (err) {
                syslog(LOG_WARNING, "In %s(): chnid=%d pthread_join() failed. tid=%ld (%s)", __func__, gs_thr_channels[i].chnid, gs_thr_channels[i].tid, strerror(err));
            }

            free(gs_thr_channels[i].p_snd_buf);
            reset_thr_chn_ent(gs_thr_channels + i);
            return 0;
        }
    }
    syslog(LOG_ERR, "In %s(): cannot find the relative channel to destroy. chnid=%d", __func__, p_listent->chnid);
    errno = EOVERFLOW;
    return -1;
}

int thr_channel_destoryall(void)
{
    int err = 0;
    for (int i = 0; i < CHNNR; ++i) {
        if (gs_thr_channels[i].chnid >= MINCHNID && gs_thr_channels[i].chnid <= MAXCHNID) {
            pthread_cancel(gs_thr_channels[i].tid);
            if (err) {
                syslog(LOG_ERR, "In %s(): chnid=%d pthread_cancel() failed. tid=%ld (%s)", __func__, gs_thr_channels[i].chnid, gs_thr_channels[i].tid, strerror(err));
                errno = err;
                return -1;
            }
            err = pthread_join(gs_thr_channels[i].tid, NULL);
            if (err) {
                syslog(LOG_WARNING, "In %s(): chnid=%d pthread_join() failed. tid=%ld (%s)", __func__, gs_thr_channels[i].chnid, gs_thr_channels[i].tid, strerror(err));
            }

            free(gs_thr_channels[i].p_snd_buf);
            memset(&gs_thr_channels[i], 0, sizeof(gs_thr_channels[i]));
            gs_thr_channels[i].chnid = -1;
            return 0;
        }
    }
    return 0;
}
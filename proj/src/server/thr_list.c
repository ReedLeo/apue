#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "thr_list.h"
#include "proto.h"
#include "server_conf.h"

// thread-id of the channel list broadcasting thread.
static pthread_t gs_tid_list_routine;

// total size of the struct msg_list_st object in bytes.
static int gs_total_bytes;

// pointer to the msg_list_st object, which content is ready to broadcast.
static struct msg_list_st* gs_p_msg_list;

static
void* list_routine(void* p_arg)
{
    int err = 0;
    while (1) {
        err = sendto(g_svfd, gs_p_msg_list, gs_total_bytes, 0, (void*)&g_svaddr, sizeof(g_svaddr));
        if (err < 0) {
            syslog(LOG_WARNING, "in %s(): call sendto failed (%s)", __func__, strerror(errno));
        } else {
            syslog(LOG_INFO, "in %s(): send program list success.", __func__);
        }
        sleep(1);
    }
    return NULL;
}

/**
 * @param:
 *  p_list: pointer to the head of list entries.
 *  list_size: the number of struct mlib_list_entry_st objects in p_list.
 * 
 * @return:
 *  0: Success;
 *  -1: On error and set errno.
*/
int thr_list_create(struct mlib_listentry_st* p_list, int list_size)
{
    int err = 0;
    struct msg_listentry_st* p_ent = NULL;

    // invalid paramters.
    if (NULL == p_list || list_size <= 0) {
        syslog(LOG_ERR, "In %s(): passed invalid parameter(s).", __func__);
        errno = EINVAL;
        return -1;
    }

    gs_total_bytes = sizeof(gs_p_msg_list->chnid);

    // count the total bytes to be sent.
    for (int i = 0; i < list_size; ++i) {
        gs_total_bytes += sizeof(struct msg_listentry_st) + strlen(p_list[i].desc);
    }

    // should be freed in thr_list_destroy()
    gs_p_msg_list = malloc(gs_total_bytes);
    if (NULL == gs_p_msg_list) {
        // the malloc has set errno.
        return -1;
    }

    // channel list is broadcasted at channel LISTCHNID.
    gs_p_msg_list->chnid = LISTCHNID;
    p_ent = gs_p_msg_list->list;
    // copy data to sending buffer.
    for (int i = 0; i < list_size; ++i) {
        p_ent->chnid = p_list[i].chnid;
        p_ent->len = htons(sizeof(*p_ent) + strlen(p_list[i].desc));
        strcpy(p_ent->desc, p_list[i].desc);
        p_ent = (void*)((char*)p_ent + p_ent->len);
    }

    err = pthread_create(&gs_tid_list_routine, NULL, list_routine, NULL);
    if (err) {
        syslog(LOG_ERR, "thr_list_create(p_list=%p, list_size=%d) failed. (%s)", p_list, list_size, strerror(err));
        errno = err;
        return -1;
    }
    return 0;
}

int thr_list_destory(void)
{
    pthread_cancel(gs_tid_list_routine);
    pthread_join(gs_tid_list_routine, NULL);
    free(gs_p_msg_list);
    return 0;
}
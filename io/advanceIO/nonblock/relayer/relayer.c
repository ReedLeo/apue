#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <assert.h>

#include "relayer.h"

enum fsm_status
{
    FSM_ST_READ=1,
    FSM_ST_WRITE,
    FSM_ST_EX,
    FSM_ST_TERM
};

#define BUFSIZE 1024
struct fsm_st
{
    int fd_src;
    int fd_dst;
    enum fsm_status status;
    int len;
    int off;
    char* errstr;
    int64_t comm_count;
    char buf[BUFSIZE];
};

struct rel_job_st
{
    int fd1;
    int fd2;
    int saved_fd_flg1;
    int saved_fd_flg2;
    int self_pos;
    enum rel_job_stat job_status;
    struct fsm_st fsm12;
    struct fsm_st fsm21;
    pthread_rwlock_t rwlock;
};

#define MAX_JOB_NUM 1024
static struct rel_job_st* gs_jobs[MAX_JOB_NUM];
static pthread_mutex_t gs_mut_jobs;

static void fsm_driver(struct fsm_st* pfsm)
{
    int byte_written = 0;

    assert(pfsm);
    switch (pfsm->status)
    {
    case FSM_ST_READ:
        pfsm->len = read(pfsm->fd_src, pfsm->buf, sizeof(pfsm->buf));
        if (pfsm->len < 0)
        {
            if (EAGAIN == errno || EINTR == errno)
                ;/* do nonting */
            else
            {   
                pfsm->errstr = "read()";
                pfsm->status = FSM_ST_EX;
            }
            
        }
        else if (pfsm->len > 0)
        {
            pfsm->status = FSM_ST_WRITE;
            pfsm->off = 0;
        }
        else    /* len == 0 */
        {
            /* read over and turn to terminating status*/
            pfsm->status = FSM_ST_TERM;
        }
        
        break;

    case FSM_ST_WRITE:
        byte_written = write(pfsm->fd_dst, pfsm->buf + pfsm->off, pfsm->len);
        if (byte_written < 0)
        {
            if (EAGAIN == errno || EINTR == errno)
                ; /* do nonthing*/
            else
            {
                pfsm->errstr = "write()";
                pfsm->status = FSM_ST_EX;
            }
        }
        else if (byte_written == pfsm->len)
        {
            pfsm->status = FSM_ST_READ;
            pfsm->len = 0;
            pfsm->off = 0;
        }
        else
        {
            /* partially written, status is still write */
            pfsm->len -= byte_written;
            pfsm->off += byte_written;
        }
        
        break;

    case FSM_ST_EX:
        perror(pfsm->errstr);
        pfsm->status = FSM_ST_TERM;
        break;
    
    case FSM_ST_TERM:
        break;

    default:
        abort();
        break;
    }
}

static void* thr_relayer(void* arg)
{
    struct rel_job_st* pcur = NULL;
    while (1)
    {
        pthread_mutex_lock(&gs_mut_jobs);
        for (int i = 0; i < MAX_JOB_NUM; ++i)
        {
            pcur = gs_jobs[i];
            if (pcur == NULL)
                continue;
            /* read lock of job entry */

            pthread_rwlock_rdlock(&pcur->rwlock);
            if (pcur->job_status == REL_JST_RUNNING)
            {
                fsm_driver(&pcur->fsm12);
                fsm_driver(&pcur->fsm21);

                // TODO
            }
            pthread_rwlock_unlock(&pcur->rwlock);
            /* release the read lock of job entry*/
        }
        pthread_mutex_unlock(&gs_mut_jobs);
    }
    pthread_exit(NULL);
}

static int get_free_pos_unlocked()
{
    for (int i = 0; i < MAX_JOB_NUM; ++i)
    {
        if (gs_jobs[i] == NULL)
            return i;
    }
    return -1;
}

int rel_init()
{
    return 0;
}

int rel_add(const char* p_dev_name1, const char* p_dev_name2)
{
    int fd1, fd2;

    if (p_dev_name1 == NULL || p_dev_name2 == NULL)
    {
        return -EINVAL;
    }

    fd1 = open(p_dev_name1, O_RDWR | O_NONBLOCK);
    if (fd1 < 0)
        return -errno;

    fd2 = open(p_dev_name2, O_RDWR | O_NONBLOCK);
    if (fd2 < 0)
        return -errno;
    
    return rel_fadd(fd1, fd2);
}

int rel_fadd(int fd1, int fd2)
{
    int pos = -1;
    struct rel_job_st* p_new_job = NULL;

    if (fd1 < 0 || fd2 < 0)
        return -EINVAL;
    
    p_new_job = malloc(sizeof(*p_new_job));
    if (p_new_job == NULL)
        return -ENOMEM;
    
    p_new_job->saved_fd_flg1 = fcntl(fd1, F_GETFL);
    if (p_new_job->saved_fd_flg1 < 0)
    {
        return errno;
    }

    p_new_job->saved_fd_flg2 = fcntl(fd2, F_GETFL);
    if (p_new_job->saved_fd_flg2 < 0)
    {
        return errno;
    }

    pthread_rwlock_init(&p_new_job->rwlock, NULL);
    p_new_job->job_status = REL_JST_RUNNING;
    p_new_job->fd1 = fd1;
    p_new_job->fd2 = fd2;

    p_new_job->fsm12.fd_src = fd1;
    p_new_job->fsm12.fd_dst = fd2;
    p_new_job->fsm12.len = 0;
    p_new_job->fsm12.off = 0;
    p_new_job->fsm12.status = FSM_ST_READ;
    p_new_job->fsm12.comm_count = 0;
    p_new_job->fsm12.buf[0] = '\0';

    p_new_job->fsm21.fd_src = fd2;
    p_new_job->fsm21.fd_dst = fd1;
    p_new_job->fsm21.len = 0;
    p_new_job->fsm21.off = 0;
    p_new_job->fsm21.status = FSM_ST_READ;
    p_new_job->fsm21.comm_count = 0;
    p_new_job->fsm21.buf[0] = '\0';
    
    /* critical section. need lock */
    pthread_mutex_lock(&gs_mut_jobs);
    pos = get_free_pos_unlocked();
    if (pos < 0)
    {
        fcntl(p_new_job->fd1, F_SETFL, p_new_job->saved_fd_flg1);
        fcntl(p_new_job->fd2, F_SETFL, p_new_job->saved_fd_flg2);
        pthread_rwlock_destroy(&p_new_job->rwlock);
        pthread_mutex_unlock(&gs_mut_jobs);
        free(p_new_job);
        return -ENOSPC;
    }
    p_new_job->self_pos = pos;
    pthread_mutex_unlock(&gs_mut_jobs);

    return pos;
}

int rel_cancel(int jid)
{
    return 0;
}

int rel_wait(int jid)
{
    return 0;
}

int rel_stat(int jid, struct rel_stat_st* p_job_stat)
{
    return 0;
}

int rel_destory(int jid)
{
    return 0;
}
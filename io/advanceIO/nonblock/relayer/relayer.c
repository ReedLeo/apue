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
    time_t start_tm;
    time_t end_tm;
    struct fsm_st fsm12;
    struct fsm_st fsm21;
    pthread_rwlock_t rwlock;
};

#define MAX_JOB_NUM 2
static struct rel_job_st* gs_jobs[MAX_JOB_NUM];
static pthread_mutex_t gs_mut_jobs = PTHREAD_MUTEX_INITIALIZER;
static pthread_t gs_tid_relayer;

#define CHECK_JD(fd) \
    if ((fd) < 0) return -EINVAL;

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

            pthread_rwlock_wrlock(&pcur->rwlock);
            if (pcur->job_status == REL_JST_RUNNING)
            {
                fsm_driver(&pcur->fsm12);
                fsm_driver(&pcur->fsm21);

                if (pcur->fsm12.status == FSM_ST_TERM && pcur->fsm21.status == FSM_ST_TERM)
                {
                    pcur->job_status = REL_JST_OVER;
                }
            }
            pthread_rwlock_unlock(&pcur->rwlock);
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

static void module_unload()
{
    pthread_cancel(gs_tid_relayer);
    pthread_join(gs_tid_relayer, NULL);
    puts("relayer: thread that drives the FSM is canceled.");
    for (int i = 0; i < MAX_JOB_NUM; ++i)
    {
        rel_destory(i);
    }
    puts("relayer: module unloaded.");
}

static void module_load()
{
    int res = 0;
    res = pthread_create(&gs_tid_relayer, NULL, thr_relayer, NULL);
    if (res < 0)
    {
        fprintf(stderr, "pthread_create faild with %s\n", strerror(res));
        exit(EXIT_FAILURE);
    }
    atexit(module_unload);
}

int rel_init()
{
    static pthread_once_t inited = PTHREAD_ONCE_INIT;
    pthread_once(&inited, module_load);
    return 0;
}


int rel_add(int fd1, int fd2)
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
        return -errno;
    }

    if (fcntl(fd1, F_SETFL, O_NONBLOCK) < 0)
    {
        return -errno;
    }

    p_new_job->saved_fd_flg2 = fcntl(fd2, F_GETFL);
    if (p_new_job->saved_fd_flg2 < 0)
    {
        return -errno;
    }

    if (fcntl(fd2, F_SETFL, O_NONBLOCK) < 0)
    {
        return -errno;
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

    p_new_job->start_tm = time(NULL);
    p_new_job->end_tm = 0;
    
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
    gs_jobs[pos] = p_new_job;
    p_new_job->self_pos = pos;
    pthread_mutex_unlock(&gs_mut_jobs);

    return pos;
}

int rel_cancel(int jid)
{
    CHECK_JD(jid);
    
    pthread_mutex_lock(&gs_mut_jobs);
    pthread_rwlock_wrlock(&gs_jobs[jid]->rwlock);
    if (gs_jobs[jid]->job_status != REL_JST_RUNNING)
    {
        pthread_rwlock_unlock(&gs_jobs[jid]->rwlock);
        pthread_mutex_unlock(&gs_mut_jobs);
        return -EBUSY;
    }
    gs_jobs[jid]->job_status = REL_JST_CANCELED;
    pthread_rwlock_unlock(&gs_jobs[jid]->rwlock);
    pthread_mutex_unlock(&gs_mut_jobs);

    return 0;
}

int rel_wait(int jid)
{
    CHECK_JD(jid);

    return 0;
}

int rel_stat(int jid, struct rel_stat_st* p_job_stat)
{  
    CHECK_JD(jid);
    if (p_job_stat == NULL)
        return 0;
    
    pthread_mutex_lock(&gs_jobs);
    if (gs_jobs[jid] == NULL)
    {
        pthread_mutex_unlock(&gs_jobs);
        return -EINVAL;
    }

    pthread_rwlock_rdlock(&gs_jobs[jid]->rwlock);
    
    p_job_stat->count12 = gs_jobs[jid]->fsm12.comm_count;
    p_job_stat->count21 = gs_jobs[jid]->fsm21.comm_count;
    
    p_job_stat->fd1 = gs_jobs[jid]->fd1;
    p_job_stat->fd2 = gs_jobs[jid]->fd2;
    
    p_job_stat->state = gs_jobs[jid]->job_status;

    p_job_stat->start_tm = gs_jobs[jid]->end_tm;
    p_job_stat->end_tm = gs_jobs[jid]->end_tm;
    
    pthread_rwlock_unlock(&gs_jobs[jid]->rwlock);
    pthread_mutex_unlock(&gs_jobs);

    return 0;
}

int rel_destory(int jid)
{
    int pos = -1;
    CHECK_JD(jid);

    pthread_mutex_lock(&gs_mut_jobs);

    pthread_rwlock_destroy(&gs_jobs[jid]->rwlock);
    free(gs_jobs[jid]);
    gs_jobs[jid] = NULL;

    pthread_mutex_unlock(&gs_mut_jobs);

    return 0;
}
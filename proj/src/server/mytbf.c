#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>
#include <syslog.h>

#include "mytbf.h"

/**
 * *********************************************************************
 * A simple implementation of token bucket for flow traffic controlling.
 * *********************************************************************
*/

#define MYTBF_MAX   1024

struct mytbf_st
{
    size_t cbs;    // Committed burst size, bucket size.
    size_t cir;    // Committed information rate, rate of token generating.
    size_t token;  // the number of tokens currently in this bucket.
    int self_pos;   // Position in mytbf_st[] array.
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

static struct mytbf_st* gs_ptr_jobs[MYTBF_MAX];
static pthread_mutex_t gs_mutex_jobs = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t gs_init_once = PTHREAD_ONCE_INIT;
static pthread_t gs_token_prod_tid;

static
int get_free_slot_unlocked(void)
{
    int pos = 0;
    while (pos < MYTBF_MAX)
    {
        if (NULL == gs_ptr_jobs[pos])
            break;
        ++pos;
    }
    return pos < MYTBF_MAX ? pos : -1;
}


/**
 * Check if the p_tbf is a valid pointer that points to a mytbf_st object.
 * This pointer must be in the internal mytbf_st*[] list.
 * @param:
 *  struct mytbf_st* p_tbf: the pointer to be checked.
 * @return:
 *  0: passed check.
 * -1: failed. p_tbf is not an valid pointer.
*/
static
int check_token_ptr_unlocked(struct mytbf_st* p_tbf)
{
    int pos = 0;
    if (NULL == p_tbf)
        return -1;
    pos = p_tbf->self_pos;
    if (pos < 0 || pos >= MYTBF_MAX || gs_ptr_jobs[pos] != p_tbf)
        return -1;
    return 0;
}


/**
 * The producer thread routine of token, which periodically generates
 * tokens for every mytbf_st object pointed by mytbf_st*[] entries.
*/
static
void* token_producer_routine(void* p_arg)
{
    while (1)
    {
        pthread_mutex_lock(&gs_mutex_jobs);
        for (int i = 0; i < MYTBF_MAX; ++i)
        {
            if (gs_ptr_jobs[i])
            {
                pthread_mutex_lock(&gs_ptr_jobs[i]->mutex);
                if (gs_ptr_jobs[i]->token <= (gs_ptr_jobs[i]->cbs - gs_ptr_jobs[i]->cir))
                {
                    gs_ptr_jobs[i]->token += gs_ptr_jobs[i]->cir;
                }
                else
                {
                    gs_ptr_jobs[i]->token = gs_ptr_jobs[i]->cbs;
                }
                // notify all the other possible token consumers.
                pthread_cond_broadcast(&gs_ptr_jobs[i]->cond);
                pthread_mutex_unlock(&gs_ptr_jobs[i]->mutex);
            }
        }
        pthread_mutex_unlock(&gs_mutex_jobs);
    }
    return NULL;
}

static
void unload_module(void)
{
    pthread_cancel(gs_token_prod_tid);
    pthread_join(gs_token_prod_tid, NULL);

    for (int i = 0; i < MYTBF_MAX; ++i)
    {
        // mytbf_destroy will get lock of gs_ptr_jobs[].
        mytbf_destroy(gs_ptr_jobs[i]);
    }

    pthread_mutex_destroy(&gs_mutex_jobs);
}

/**
 * Initialize mytbf module onece. Creating the token producer thread.
*/
static
void load_module(void)
{
    int err = 0;
    err = pthread_create(&gs_token_prod_tid, NULL, token_producer_routine, NULL);
    if (err)
    {
        syslog(LOG_ERR, "mbtf create token producer failed.(%s)", strerror(err));
        exit(EXIT_FAILURE);
    }
    atexit(unload_module);
}

/**
 * @param:
 *  int cir: Committed Information Rate.
 *           承诺信息速率， 令牌生成速率。
 *  int cbs: Committed Burst Size. 
 *           承诺突发尺寸, 即最大瞬时传输量， 令牌桶深度。
 *  cbs >= cir > 0
 * @return:
 *  NULL: failed. set errno.
 *  Not NULL: a pointer to mytbf_st object, but convert to mytbf_t*.
*/
mytbf_t* mytbf_init(const size_t cir, const size_t cbs)
{
    struct mytbf_st* p_new_tbf = NULL;

    pthread_once(&gs_init_once, load_module);

    if (cir == 0 || cbs == 0 || (cir > cbs))
    {
        errno = EINVAL;
        return NULL;
    }

    p_new_tbf = malloc(sizeof(*p_new_tbf));
    if (p_new_tbf)
    {
        p_new_tbf->cir = cir;
        p_new_tbf->cbs = cbs;
        pthread_mutex_init(&p_new_tbf->mutex, NULL);
        pthread_cond_init(&p_new_tbf->cond, NULL);

        pthread_mutex_lock(&gs_mutex_jobs);
        int pos = get_free_slot_unlocked();
        if (pos < 0)
        {   
            // malloc failed. destroy mutex, conditional variables and return NULL.
            pthread_mutex_destroy(&p_new_tbf->mutex);
            pthread_cond_destroy(&p_new_tbf->cond);
            free(p_new_tbf);
            pthread_mutex_unlock(&gs_mutex_jobs);
            return NULL;
        }
        gs_ptr_jobs[pos] = p_new_tbf;
        p_new_tbf->self_pos = pos;
        pthread_mutex_unlock(&gs_mutex_jobs);
    }
    return p_new_tbf;
}


/**
 * @param:
 *  mytbf_t* p_token: points to mytbf_t object.
 *  int requested_token_num:  the number of requested tokens.
 * @return:
 *  -1: error and set errno
 *  non-negtive: the number of token was actually taken.
*/
ssize_t mytbf_fetch_token(mytbf_t* p_token, const size_t requested_token_num)
{
    ssize_t really_taken_num = 0;
    struct mytbf_st* p_tbf = p_token;
    pthread_mutex_lock(&gs_mutex_jobs);
    if (check_token_ptr_unlocked(p_tbf) < 0)
    {
        pthread_mutex_unlock(&gs_mutex_jobs);
        errno = EINVAL;
        return -1;
    }
    pthread_mutex_unlock(&gs_mutex_jobs);

    pthread_mutex_lock(&p_tbf->mutex);
    while (p_tbf->token <= 0)
        pthread_cond_wait(&p_tbf->cond, &p_tbf->mutex);
    
    really_taken_num = (p_tbf->token < requested_token_num) ? p_tbf->token : requested_token_num;
    p_tbf->token -= really_taken_num;
    pthread_mutex_unlock(&p_tbf->mutex);

    return really_taken_num;
}

/**
 * @param:
 *  mytbf_t* p_token: A pointer points to mytbf_st object and should be placed in 
 *      the internal mytbf_st*[] list.
 *  size_t token_num_to_return: The number of toekns you want to give back.
 * 
 * @return:
 *  0: Success.
 *  -1: Failed and set errno.
*/
ssize_t mytbf_return_token(mytbf_t* p_token, const size_t token_num_to_return)
{
    struct mytbf_st* p_tbf = p_token;

    pthread_mutex_lock(&gs_mutex_jobs);
    if (check_token_ptr_unlocked(p_tbf) < 0)
    {
        errno = EINVAL;
        pthread_mutex_unlock(&gs_mutex_jobs);
        return -1;
    }
    pthread_mutex_unlock(&gs_mutex_jobs);

    pthread_mutex_lock(&p_tbf->mutex);
    if (token_num_to_return > (p_tbf->cbs - p_tbf->token))
    {
        p_tbf->token = p_tbf->cbs;
    }
    else
    {
        p_tbf->token += token_num_to_return;
    }
    // notify the possible consumers that tokens are avalibale.
    pthread_cond_broadcast(&p_tbf->cond);
    pthread_mutex_unlock(&p_tbf->mutex);

    return 0;
}


/**
 * @param：
 *  mytbf_t* p_token: pointer to mytbf_st object. And this pointer also stores in internal list.
 * @return
 *  0: Success.
 *  -1: Error and set errno.
*/
ssize_t mytbf_destroy(mytbf_t* p_token)
{
    struct mytbf_st* p_tbf = (struct mytbf_st*) p_token;
    int pos = p_tbf->self_pos;
    // no need actually free.
    if (NULL == p_tbf)
        return 0;
    
    // self_pos must be valid and p_token, gs_ptr_jobs[pos] must points to the same address.
    if (pos < 0 || pos >= MYTBF_MAX || p_tbf != gs_ptr_jobs[pos])
    {
        errno = EINVAL;
        return -1;
    }

    pthread_mutex_lock(&gs_mutex_jobs);
    if (gs_ptr_jobs[pos])
    {
        gs_ptr_jobs[pos] = NULL;
    }
    else
    {
        // gs_ptr_jobs[pos]==NULL. Not a valid mytbf_st 
        // entry in jobs[]. do nonthing on p_token.
        pthread_mutex_unlock(&gs_mutex_jobs);
        return 0;
    }
    pthread_mutex_unlock(&gs_mutex_jobs);

    pthread_mutex_destroy(&p_tbf->mutex);
    pthread_cond_destroy(&p_tbf->cond);
    free(p_tbf);
    return 0;
}
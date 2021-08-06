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

#include "mytbf.h"

/**
 * *********************************************************************
 * A simple implementation of token bucket for flow traffic controlling.
 * *********************************************************************
*/

#define MYTBF_MAX   1024

struct mytbf_st
{
    int cbs;    // Committed burst size, bucket size.
    int cir;    // Committed information rate, rate of token generating.
    int token;  // the number of tokens currently in this bucket.
    int self_pos;   // Position in mytbf_st[] array.
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

static struct mytbf_st* gs_ptr_jobs[MYTBF_MAX];
static pthread_mutex_t gs_mutex_jobs = PTHREAD_MUTEX_INITIALIZER;

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
 * @param:
 *  int cir: Committed Information Rate.
 *           承诺信息速率， 令牌生成速率。
 *  int cbs: Committed Burst Size. 
 *           承诺突发尺寸, 即最大瞬时传输量， 令牌桶深度。
 * @return:
 *  NULL: failed. set errno.
 *  Not NULL: a pointer to mytbf_st object, but convert to mytbf_t*.
*/
mytbf_t* mytbf_init(int cir, int cbs)
{
    struct mytbf_st* p_new_tbf = NULL;
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
int mytbf_fetch_token(mytbf_t* p_token, const int requested_token_num)
{
    int really_taken_num = 0;
    struct mytbf_st* p_tbf = p_token;
    pthread_mutex_lock(&gs_mutex_jobs);
    if (check_token_ptr_unlocked(p_tbf) < 0)
    {
        pthread_mutex_unlock(&gs_ptr_jobs);
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

int mytbf_return_token(mytbf_t* p_token, int i)
{
    // TODO
    return 0;
}


/**
 * @param：
 *  mytbf_t* p_token: pointer to mytbf_st object. And this pointer also stores in internal list.
 * @return
 *  0: Success.
 *  -1: Error and set errno.
*/
int mytbf_destroy(mytbf_t* p_token)
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

    pthread_mutex_lock(&gs_mutex_jobs[pos]);
    if (gs_ptr_jobs[pos])
    {
        gs_ptr_jobs[pos] = NULL;
    }
    else
    {
        // gs_ptr_jobs[pos]==NULL. Not a valid mytbf_st 
        // entry in jobs[]. do nonthing on p_token.
        pthread_mutex_unlock(&gs_mutex_jobs[pos]);
        return 0;
    }
    pthread_mutex_unlock(&gs_mutex_jobs[pos]);

    pthread_mutex_destroy(&p_tbf->mutex);
    pthread_cond_destroy(&p_tbf->cond);
    free(p_tbf);
    return 0;
}
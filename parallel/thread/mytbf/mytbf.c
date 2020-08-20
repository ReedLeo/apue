/**
 * 2020-08-16
 * 实现令牌桶流控
 * 
 * 2020年8月20日
 * 用pthread线程并发机制取代信号机制
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>

#include "mytbf.h"

#define CHECK_PTR(ptbf) \
if (ptbf == NULL)\
{\
    fprintf(stderr, "%s: pointer of mytbf_t* CANNOT be NULL.\n", __FUNCTION__);\
    exit(1);\
}

typedef struct mytbf_st
{
    int mt_cps;
    int mt_burst;
    int mt_token;
    int mt_self_pos;
    pthread_mutex_t mt_mut_lock;
    pthread_cond_t mt_con_lock;
} mytbf_st;

static mytbf_st* gs_tkbucket_arr[MYTBF_MAX];
static pthread_mutex_t gs_mut_task = PTHREAD_MUTEX_INITIALIZER;
static pthread_t gs_tid_token_generater;
static struct timespec gs_st_token_gen_itv;

static int get_free_pos_unlocked(void)
{
    for (int i = 0; i < MYTBF_MAX; ++i)
    {
        if (!gs_tkbucket_arr[i])
            return i;
    }
    return -1;
}

static int min(int a, int b)
{
    return a < b ? a : b;
}

static void* thr_token_generater(void* arg)
{
    mytbf_st* p_tbf = NULL;
    struct timespec st_rem_itv;
    int err = 0;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    while (1)
    {
        pthread_mutex_lock(&gs_mut_task);
        for (int i = 0; i < MYTBF_MAX; ++i)
        {
            p_tbf = gs_tkbucket_arr[i];
            if (p_tbf)
            {
                pthread_mutex_lock(&p_tbf->mt_mut_lock);
                p_tbf->mt_token += p_tbf->mt_cps;
                if (p_tbf->mt_token > p_tbf->mt_burst)
                    p_tbf->mt_token = p_tbf->mt_burst;
                pthread_cond_signal(&p_tbf->mt_con_lock);
                pthread_mutex_unlock(&p_tbf->mt_mut_lock);
            }
        }
        pthread_mutex_unlock(&gs_mut_task);

        if (nanosleep(&gs_st_token_gen_itv, &st_rem_itv))
        {
            err = errno;
            if (err == EINTR)
                nanosleep(&st_rem_itv, NULL);
            else
            {
                fprintf(stderr, "nanosleep failed with \"%s\"\n", strerror(err));
                break;
            }
        }
    }
    pthread_exit(NULL);
}

static void module_unload(void)
{
    int err;
    
    if (err = pthread_cancel(gs_tid_token_generater))
    {
        fprintf(stderr, "pthread_cancel failed with \"%s\"\n", strerror(err));
        exit(EXIT_FAILURE);
    }
    
    if (err = pthread_join(gs_tid_token_generater, NULL))
    {
        fprintf(stderr, "pthread_join failed with \"%s\"\n", strerror(err));
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MYTBF_MAX; ++i)
    {
        mytbf_destory(gs_tkbucket_arr[i]);
    }
}

static void module_load(void)
{
    int err = 0;
    // configure the token generation's interval.
    gs_st_token_gen_itv.tv_nsec = 0;
    gs_st_token_gen_itv.tv_sec = 1;

    err = pthread_create(&gs_tid_token_generater, NULL, thr_token_generater, NULL);
    if (err)
    {
        fprintf(stderr, "module_load() failed to create thread by pthrea_create. Error message: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }
    atexit(module_unload);
}

mytbf_t* mytbf_init(int cps, int burst)
{
    static pthread_once_t init_ctl = PTHREAD_ONCE_INIT;
    mytbf_st* p_cur;
    int pos = -1;
    int err = 0;
    
    if (cps < 0 || burst < 0)
        return NULL;
    
    pthread_once(&init_ctl, module_load);

    p_cur = malloc(sizeof(mytbf_st));
    if (p_cur == NULL)
        return NULL;
    p_cur->mt_burst = burst;
    p_cur->mt_cps = cps;
    p_cur->mt_token = 0;

    err = pthread_cond_init(&p_cur->mt_con_lock, NULL);
    if (err)
    {
        fprintf(stderr, "pthread_cond_init faied. Error message: %s\n", strerror(err));
        free(p_cur);
        return NULL;
    }
    
    err = pthread_mutex_init(&p_cur->mt_mut_lock, NULL);
    if (err)
    {
        fprintf(stderr, "pthread_mutex_init failed. Error message: %s", strerror(err));
        pthread_cond_destroy(&p_cur->mt_con_lock);
        free(p_cur);
        return NULL;
    }

    pthread_mutex_lock(&gs_mut_task);
    pos = get_free_pos_unlocked();
    if (pos < 0)
    {
        pthread_mutex_unlock(&gs_mut_task);
        free(p_cur);
        return NULL;
    }
    p_cur->mt_self_pos = pos;
    gs_tkbucket_arr[pos] = p_cur;
    pthread_mutex_unlock(&gs_mut_task);

    return p_cur;
}

int mytbf_fetch_token(mytbf_t* p_tbf, int request)
{   
    int token_got = 0;
    CHECK_PTR(p_tbf);
    mytbf_st* p_cur = (mytbf_st*)p_tbf;

    if (request <= 0)
        return -EINVAL;

    pthread_mutex_lock(&p_cur->mt_mut_lock);
    
    while (p_cur->mt_token <= 0)
        pthread_cond_wait(&p_cur->mt_con_lock, &p_cur->mt_mut_lock);
    
    token_got = min(request, p_cur->mt_token);
    p_cur->mt_token -= token_got;

    pthread_mutex_unlock(&p_cur->mt_mut_lock);

    return token_got;
}

int mytbf_return_token(mytbf_t* p_tbf, int token_num)
{
    mytbf_st* p_cur;
    CHECK_PTR(p_tbf);

    p_cur = (mytbf_st*)p_tbf;
    pthread_mutex_lock(&p_cur->mt_mut_lock);

    p_cur->mt_token += token_num;
    if (p_cur->mt_token > p_cur->mt_burst)
        p_cur->mt_token = p_cur->mt_burst;
    pthread_cond_signal(&p_cur->mt_con_lock);
    
    pthread_mutex_unlock(&p_cur->mt_mut_lock);
    
    return token_num;
}

int mytbf_destory(mytbf_t* p_tbf)
{
    mytbf_st* p_cur = (mytbf_st*)p_tbf;

    if (!p_cur)
        return -EINVAL;

    pthread_mutex_lock(&gs_mut_task);
    gs_tkbucket_arr[p_cur->mt_self_pos] = NULL;
    pthread_mutex_unlock(&gs_mut_task);
    
    pthread_mutex_destroy(&p_cur->mt_mut_lock);
    pthread_cond_destroy(&p_cur->mt_con_lock);
    free(p_cur);
    

    return 0;
}

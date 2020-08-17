/**
 * 2020-08-16
 * 实现令牌桶流控
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

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
} mytbf_st;

static mytbf_st* gs_tkbucket_arr[MYTBF_MAX];
static sig_t gs_old_alrm_handler;

static int get_free_pos(void)
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

static void alrm_handler(int arg)
{
    mytbf_st* p_tbf = NULL;
    alarm(1);
    for (int i = 0; i < MYTBF_MAX; ++i)
    {
        p_tbf = gs_tkbucket_arr[i];
        if (p_tbf)
        {
            p_tbf->mt_token += p_tbf->mt_cps;
            if (p_tbf->mt_token > p_tbf->mt_burst)
                p_tbf->mt_token = p_tbf->mt_burst;
        }
    }
}

static void module_unload(void)
{
    signal(SIGALRM, gs_old_alrm_handler);
    alarm(0);
    for (int i = 0; i < MYTBF_MAX; ++i)
        free(gs_tkbucket_arr[i]);
}

static void module_load(void)
{
    gs_old_alrm_handler = signal(SIGALRM, alrm_handler);
    alarm(1);
    atexit(module_unload);
}

mytbf_t* mytbf_init(int cps, int burst)
{
    static int init = 0;
    mytbf_st* p_cur;
    int pos = -1;
    
    if (cps < 0 || burst < 0)
        return NULL;
    
    p_cur = malloc(sizeof(mytbf_st));
    if (p_cur == NULL)
        return NULL;

    pos = get_free_pos();
    if (pos < 0)
        return NULL;
    
    p_cur->mt_burst = burst;
    p_cur->mt_cps = cps;
    p_cur->mt_token = 0;
    p_cur->mt_self_pos = pos;
    
    gs_tkbucket_arr[pos] = p_cur;

    if (!init)
    {
        module_load();
        init = 1;
        
    }

    return p_cur;
}

int mytbf_fetch_token(mytbf_t* p_tbf, int request)
{   
    int token_got = 0;
    CHECK_PTR(p_tbf);
    mytbf_st* p_cur = (mytbf_st*)p_tbf;

    if (request <= 0)
        return -EINVAL;
    
    while (p_cur->mt_token <= 0)
        pause();

    token_got = min(request, p_cur->mt_token);
    p_cur->mt_token -= token_got;

    return token_got;
}

int mytbf_return_token(mytbf_t* p_tbf, int token_num)
{
    CHECK_PTR(p_tbf);
    return 0;
}

int mytbf_destory(mytbf_t* p_tbf)
{
    mytbf_st* p_cur = (mytbf_st*)p_tbf;

    if (!p_cur)
        return -EINVAL;
    
    gs_tkbucket_arr[p_cur->mt_self_pos] = NULL;
    free(p_cur);

    return 0;
}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "anytimer.h"

#define CHECK_PTR(ptr)\
if (ptr == NULL) \
{\
    fprintf(stderr, "%s: task pointer cannot be null.\n", __FUNCTION__);\
    exit(EXIT_FAILURE);\
}

typedef struct tmtask_st
{
    int itv_sec;    // interval in second
    int count_down; // count down in second
    anytimer_handler_t handler;
    int task_enable;
    int self_pos;
} tmtask_st;

static tmtask_st* gs_tasks_tbl[TASK_MAX_NUM];
static sig_t gs_old_sighandler;
static int gs_inited;

static int get_free_pos(void)
{
    for (int i = 0; i < TASK_MAX_NUM; ++i)
    {
        if (!gs_tasks_tbl[i])
            return i;
    }
    return -1;
}

static void task_sched(int signum)
{
    tmtask_st* p_task = NULL;

    for (int i = 0; i < TASK_MAX_NUM; ++i)
    {
        if (gs_tasks_tbl[i] && gs_tasks_tbl[i]->task_enable)
        {
            p_task = gs_tasks_tbl[i];
            --p_task->count_down;
            if (p_task->count_down <= 0)
            {
                p_task->handler(signum);
                p_task->count_down = p_task->itv_sec;
            }
        }
    }
    alarm(1);
}

static void module_unload(void)
{
    signal(SIGALRM, gs_old_sighandler);
    alarm(0);
    for (int i = 0; i < TASK_MAX_NUM; ++i)
    {
        free(gs_tasks_tbl[i]);
        gs_tasks_tbl[i] = NULL;
    }
}

static void module_load(void)
{
    gs_old_sighandler = signal(SIGALRM, task_sched);
    alarm(1);
    atexit(module_unload);
}

int anytimer_init()
{
    if (!gs_inited)
    {
        module_load();
        gs_inited = 1;
    }
    return 0;
}

anytask_t* anytimer_add_task(anytimer_handler_t const handler, const int interval, const int enable)
{
    tmtask_st* p_task = NULL;
    int pos = -1;

    if (handler == NULL || interval < 0)
        return NULL;

    pos = get_free_pos();
    if (pos < 0)
        return NULL;
    
    p_task = malloc(sizeof(tmtask_st));
    if (p_task == NULL)
    {
        return NULL;
    }

    gs_tasks_tbl[pos] = p_task;
    p_task->itv_sec = interval;
    p_task->count_down = interval;
    p_task->handler = handler;
    p_task->self_pos = pos;
    p_task->task_enable = enable;
    
    return p_task;
}

/**
 *  0 -- disable
 *  non-zero -- enable 
 */
int anytimer_enable_task(anytask_t* const p_task, const int enable)
{
    tmtask_st *ptsk = p_task;
    CHECK_PTR(ptsk);
    ptsk->task_enable = enable;
    return 0;
}

int anytimer_remove_task(anytask_t* const p_task)
{
    tmtask_st *ptsk = p_task;
    CHECK_PTR(ptsk);
    gs_tasks_tbl[ptsk->self_pos] = NULL;
    free(ptsk);
    ptsk = NULL;
    return 0;
}

void anytimer_destory()
{
    if (gs_inited)
    {
        module_unload();
        gs_inited = 0;
    }
}
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "anytimer.h"

#define CHECK_PTR(ptr)\
if (ptr == NULL) \
{\
    fprintf(stderr, "%s: task pointer cannot be null.\n", __FUNCTION__);\
    exit(EXIT_FAILURE);\
}

#define IS_RUNNABLE(ptsk) ((ptsk->task_status) & (AT_ST_RUNNABLE))
#define IS_CANCELED(ptsk) ((ptsk->task_status) & (AT_ST_CANCEL))
#define IS_PAUSED(ptsk)   ((ptsk->task_status) & (AT_ST_PAUSE))

typedef struct tmtask_st
{
    int count_down; // count down in second.
    anytimer_handler_t handler;
    void* args;     // handler's argument.
    int task_status;
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
        if (gs_tasks_tbl[i] && IS_RUNNABLE(gs_tasks_tbl[i]))
        {
            p_task = gs_tasks_tbl[i];
            --p_task->count_down;
            if (p_task->count_down <= 0)
            {
               p_task->handler(p_task->args);
               p_task->task_status = AT_ST_CANCEL;
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

int at_init()
{
    if (!gs_inited)
    {
        module_load();
        gs_inited = 1;
    }
    return 0;
}

int at_add_task(anytimer_handler_t const handler, const int interval, const int status)
{
    return 0;
}

/**
 *  0 -- disable
 *  non-zero -- enable 
 */
int at_pause_task(int td)
{
    return 0;
}

int at_resume_task(int td)
{
    return 0;
}

int at_cancel_task(const int td)
{
    return 0;
}

int at_wait_task(const int td)
{
    return 0;
}

int at_destory()
{
    if (gs_inited)
    {
        module_unload();
        gs_inited = 0;
        return 0;
    }
    else
    {
        return -EAGAIN;
    }
    
}
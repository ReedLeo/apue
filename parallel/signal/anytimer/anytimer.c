#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "anytimer.h"

#define CHECK_TD(td)\
if (td < 0) \
{\
    fprintf(stderr, "%s: invalid task descriptor.\n", __FUNCTION__);\
    return -EINVAL;\
}

#define IS_RUNNABLE(ptsk) ((ptsk->task_status) & (AT_ST_RUNNABLE))
#define IS_CANCELED(ptsk) ((ptsk->task_status) & (AT_ST_CANCEL))
#define IS_PAUSED(ptsk)   ((ptsk->task_status) & (AT_ST_PAUSE))

#define SIGTSKFIN (SIGRTMIN + 1)

typedef struct tmtask_st
{
    int count_down; // count down in second.
    at_handler_t handler;
    void* args;     // handler's argument.
    int task_status;
    int self_pos;
} tmtask_st;

static tmtask_st* gs_tasks_tbl[TASK_MAX_NUM];
static sig_t gs_old_alrm_handler;
static sig_t gs_old_fintsk_handler;
static sigset_t gs_block_set;
static sigset_t gs_old_set;
static sigset_t gs_saved_set;
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
               raise(SIGTSKFIN);
            }
        }
    }
    alarm(1);
}

static void module_unload(void)
{   
    sigprocmask(SIG_SETMASK, &gs_saved_set, NULL);
    signal(SIGTSKFIN, gs_old_fintsk_handler);
    signal(SIGALRM, gs_old_alrm_handler);
    alarm(0);
    for (int i = 0; i < TASK_MAX_NUM; ++i)
    {
        free(gs_tasks_tbl[i]);
        gs_tasks_tbl[i] = NULL;
    }
}

static void module_load(void)
{
    gs_old_fintsk_handler = signal(SIGTSKFIN, SIG_IGN);
    sigemptyset(&gs_block_set);
    sigaddset(&gs_block_set, SIGTSKFIN);
    sigprocmask(SIG_UNBLOCK, &gs_block_set, &gs_saved_set);
    if (sigprocmask(SIG_BLOCK, &gs_block_set, &gs_old_set) < 0)
    {
        perror("sigprocmask()");
        exit(EXIT_FAILURE);
    }
    gs_old_alrm_handler = signal(SIGALRM, task_sched);
    alarm(1);
    atexit(module_unload);
}

static int rm_task(const int td)
{
    tmtask_st* ptsk = NULL;
    CHECK_TD(td);

    ptsk = gs_tasks_tbl[td];
    gs_tasks_tbl[td] = 0;
    free(ptsk);
    return td;
}

int at_init()
{
    if (!gs_inited)
    {
        module_load();
        gs_inited = 1;
        return 0;
    }
    else
    {
        return -EAGAIN;
    }
    
}

int at_add_task(at_handler_t const handler, void* args, const int interval, at_status_enum status)
{
    int td = -1;
    tmtask_st* ptsk = NULL;

    if (handler == NULL || interval <= 0 
        || (status != AT_ST_CANCEL 
            && status != AT_ST_RUNNABLE
            && status != AT_ST_PAUSE)
        )
        return -EINVAL;

    td = get_free_pos();
    if (td < 0)
    {
        return -EXFULL;
    }

    ptsk = malloc(sizeof(*ptsk));
    if (ptsk == NULL)
    {
        return -ENOMEM;
    }
    gs_tasks_tbl[td] = ptsk;

    ptsk->handler = handler;
    ptsk->args = args;
    ptsk->count_down =interval;
    ptsk->self_pos = td;
    ptsk->task_status = status;

    return td;
}

/**
 *  0 -- disable
 *  non-zero -- enable 
 */
int at_pause_task(int td)
{
    tmtask_st* ptsk = NULL;
    CHECK_TD(td);
    
    ptsk = gs_tasks_tbl[td];
    if (ptsk == NULL || IS_CANCELED(ptsk))
    {
        return -EINVAL;
    }

    if (IS_PAUSED(ptsk))
        return -EAGAIN;
    
    ptsk->task_status = AT_ST_PAUSE;
    return 0;
}

int at_resume_task(int td)
{
    tmtask_st* ptsk = NULL;
    CHECK_TD(td);
    
    ptsk = gs_tasks_tbl[td];
    if (ptsk == NULL || IS_CANCELED(ptsk))
    {
        return -EINVAL;
    }
    
    ptsk->task_status = AT_ST_RUNNABLE;
    return 0;
}

int at_cancel_task(const int td)
{
    tmtask_st* ptsk = NULL;
    CHECK_TD(td);
    
    ptsk = gs_tasks_tbl[td];
    if (ptsk == NULL)
    {
        return -EINVAL;
    }

    if (IS_CANCELED(ptsk))
        return -EAGAIN;
    
    ptsk->task_status = AT_ST_CANCEL;
    return 0;
}

int at_wait_task(const int td)
{
    tmtask_st* ptsk = NULL;
    CHECK_TD(td);
    
    ptsk = gs_tasks_tbl[td];
    if (ptsk == NULL)
    {
        return -EINVAL;
    }

    while (ptsk->task_status != AT_ST_CANCEL)
        sigsuspend(&gs_old_set);    // how to wake up this pause()
    //sigprocmask(SIG_SETMASK, &gs_saved_set, NULL);
    return rm_task(td);
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
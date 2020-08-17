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


/**
 * Task schedule procedure, driven by alarm(1).
*/
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
               /**
                * !! A real-time signal dose not lose !!
                * raise a self-defined real-time signal.
                * because we've blocked this signal, it 
                * will pend and until the at_wait_task()
                * consume it.
               */
               raise(SIGTSKFIN);
            }
        }
    }
    alarm(1);
}

static void module_unload(void)
{   
    /* recover signal mask and signal handlers to the orignal
    *   status before loading this module.
    */
    sigprocmask(SIG_SETMASK, &gs_saved_set, NULL);
    signal(SIGTSKFIN, gs_old_fintsk_handler);
    signal(SIGALRM, gs_old_alrm_handler);
    alarm(0);   // clean the alarm
    for (int i = 0; i < TASK_MAX_NUM; ++i)
    {
        free(gs_tasks_tbl[i]);
        gs_tasks_tbl[i] = NULL;
    }
}

static void module_load(void)
{
    /* 
    *  When receiving a SIGTSKFIN real-time
    *  signal, current thread dosen't need to have handler to 
    *  process it or exit. Ignoring it will be fine.
    */ 
    gs_old_fintsk_handler = signal(SIGTSKFIN, SIG_IGN);
    sigemptyset(&gs_block_set); // init sigset to empty.
    sigaddset(&gs_block_set, SIGTSKFIN);    // add our-self defined real-time signal.
    /**
     * If there already have blocked the real-time signal we defined, 
     * we unblock it first and save the original sigset to gs_saved_set
     * for recovering.
    */
    if ( (sigprocmask(SIG_UNBLOCK, &gs_block_set, &gs_saved_set) < 0)
      || (sigprocmask(SIG_BLOCK, &gs_block_set, &gs_old_set) < 0)
     )
    {
        perror("sigprocmask()");
        exit(EXIT_FAILURE);
    }
    /**
     * save original SIGALRM's handler and set a new one.
    */
    gs_old_alrm_handler = signal(SIGALRM, task_sched);
    alarm(1);
    atexit(module_unload);
}


/**
 * Remove a task identified by descriptor {int}td.
 * Return value: {int}
 *  td ,on success, is the removed task's descriptor.
 *  -EINVAL, on falied, because the descriptor td is invalid.
*/
static int rm_task(const int td)
{
    tmtask_st* ptsk = NULL;
    CHECK_TD(td);

    ptsk = gs_tasks_tbl[td];
    gs_tasks_tbl[td] = 0;
    free(ptsk);
    return td;
}

/**
 * Initialize anytimer.
 * Return value {int}:
 *  0, on success;
 *  -EAGAIN, already initilized.
*/
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

/**
 * Add a anytimer task into task list. 
 * Usually we use AT_ST_RUNNABLE as the status argument, after interval seconds late, 
 * the task handler will be executed and its status will be changed to AT_ST_CANCEL.
 * If status ot the task is not AT_ST_RUNNABLE, it will not count down for this task until
 * its status being changed to AT_ST_RUNNABLE.
 * 
 * Return value:
 *  non-negtive, on success, is the descriptor of the task we just added.
 *  -EINVAL, some arguments are invalid.
 *  -EXFULL, there isn't space in task descriptor table.
 *  -ENOMEM, there isn't enough heap memory in system.
*/
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
 * Pause the task indentifyed by td.
 * 
 * Return value:
 *  0, on success.
 *  -EINVAL, td is invalid.
 *  -EINVAL, the task indentifyed by td alredy been canceled.
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

/**
 * Resume a task identifyed by td.
 * 
 * Return value:
 *  0, on success.
 *  -EINVAL, ts is invalid.
*/
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


/**
 * Cancel a task identifyed by td
 * 
 * Return value:
 *  0, on success.
 *  -EINVAL, td is invalid.
 *  -EAGAIN, the task has already been canceled.
 */
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


/**
 * Wait a task to terminate. This function dosen't return until the 
 * task is terminated.
 * 
 * Return value:
 *  non-negtive, on success, is the td we waited for.
 *  -EINVAL, td is invalid.
*/
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


/*
* Destory anytimer and release resources.
*  
* Return value:
*  0, on success.
*  -EAGAIN, already destoryed.
*/
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
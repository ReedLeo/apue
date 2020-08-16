#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "anytimer.h"

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

static void task_sched(int signum)
{
    tmtask_st* p_task = NULL;

    for (int i = 0; i < TASK_MAX_NUM; ++i)
    {
        if (gs_tasks_tbl[i] || gs_tasks_tbl[i]->task_enable)
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

int anytimer_init()
{
    static int inited = 0;
    if (!inited)
    {
        gs_old_sighandler = signal(SIGALRM, task_sched);
        alarm(1);
    }
    return 0;
}

anytask_t* anytimer_add_task(anytimer_handler_t handler, int interval, int enable)
{
    tmtask_st* p_task = NULL;
    int pos = -1;

    pos = get_free_pos();
    if (pos < 0)
        return NULL;
    
    p_task = gs_tasks_tbl[pos];
    p_task->count_down = 0;
    p_task->handler = handler;
    p_task->itv_sec = interval;
    p_task->self_pos = pos;
    p_task->task_enable = enable;

    return p_task;
}

/**
 *  0 -- disable
 *  1 -- enable 
 */
int anytimer_enable_task(anytask_t* p_task, int enable)
{

}

int anytimer_remove_task(anytask_t* p_task)
{

}

int anytimer_destory()
{

}
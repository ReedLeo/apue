#ifndef ANYTIMER_H__
#define ANYTIMER_H__

#define TASK_MAX_NUM 1024

typedef void (*anytimer_handler_t)(int);
typedef void anytask_t;

int anytimer_init();
anytask_t* anytimer_add_task(anytimer_handler_t handler, const int inverval, const int enable);

/**
 *  0 -- disable
 *  1 -- enable 
 */
int anytimer_enable_task(anytask_t* const p_task, const int enable);

int anytimer_remove_task(anytask_t* const p_task);

void anytimer_destory();

#endif
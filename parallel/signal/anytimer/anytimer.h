#ifndef ANYTIMER_H__
#define ANYTIMER_H__

#define TASK_MAX_NUM 1024

#define AT_ST_PAUSE     0
#define AT_ST_RUNNABLE  1
#define AT_ST_CANCEL    2

typedef void (*anytimer_handler_t)(void*);

/**
 * @description: 
 * @param {void} 
 * @return {int}
 *      0, success
 *      -EAGAIN, has inited.
 */
int at_init();

/**
 * @description: Try to add a new task for scheduling.
 * @param :
 *      anytimer_handler_t handler, the procedure of task
 *      interval, time in second before the handler begin to run.
 *      status, the inital status of task, which can be AT_ST_PAUSE, AT_ST_RUNNABLE or AT_ST_CANCEL
 * @return {int}  
 *      non-negtive, task descriptor
 *      -ENOMEM, There is no space for new task.
 *      -EXFULL, The task descriptor table is full.
 */
int at_add_task(anytimer_handler_t handler, const int inverval, const int status);

/**
 * @description: 
 * @param {int} Task descriptor of the task to pause.
 * @return {int} 
 *      Original task descriptor on success.
 *      -EINVAL, Task descriptor invalid.
 *      -EAGAIN, Task had been paused.
 */
int at_pause_task(const int td);


/**
 * @description: 
 * @param {int} Task descriptor of the task to resume.
 * @return {int} 
 *      Original task descriptor on success.
 *      -EINVAL, Task descriptor invalid.
 *      -EAGAIN, Task had been resumed.
 */
int at_resume_task(const int td);


/**
 * @description: 
 * @param {int} Task descriptor of the task to cancel.
 * @return {int}
 *      Original task descriptor on success.
 *      -EINVAL, Task descriptor invalid.
 *      -EAGAIN, Task had been canceled. 
 */
int at_cancel_task(const int td);


/**
 * @description: wait fo an task by task descriptor to terminate and release its resources.
 * @param {int} Task descriptor of the task to cancel.
 * @return {int} 
 *      Original task descriptor on success.
 *      -EINVAL, Task descriptor invalid.
 */
int at_wait_task(int td);

/**
 * @description: Destory anytimer module, cancel all tasks and realease their resources.
 * @param {void} 
 * @return {int} 
 *      0 on Success.
 *      -EAGAIN, destory again or dosen't initialized yet.
 */
int at_destory();

#endif
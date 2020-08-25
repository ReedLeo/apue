#ifndef __RELAYER_H__
#define __RELAYER_H__

#include <time.h>
#include <stdint.h>

enum rel_job_stat
{
    REL_JST_RUNNING = 1,
    REL_JST_CANCELED,
    REL_JST_OVER
};

/* expose to user */
struct rel_stat_st
{
    int state;
    int fd1;
    int fd2;
    int64_t count12;
    int64_t count21;
    time_t start_tm;
    time_t end_tm;
};

int rel_init();

/**
 * Description:
 *  Add a new relayer job, and returen the job descriptor if it succeeded.
 *  rel_add() need 2 opened file's descriptor.
 *  if failed, it would return a negtive value of error number.
 * 
 * Return value:
 *  non-negtive, on success, the relayer job descriptor
 *  -EINVAL, if either device name is invalid.
 *  -ENOMEM, not enough heap memory.
 *  -ENOSPC, the job descriptor table is full.
 *  and other negtive value of error number.
*/
int rel_add(int fd1, int fd2);


/**
 * Description:
 *  Try to cancel a job by job descriptor jid.
 *
 * Return value:
 *  0, on success.
 *  -EINVAL, if job id is invalid.
 *  -EBUSY, the job has already been canceld.
*/
int rel_cancel(int jid);


/**
 * Description:
 *  Wait for a job to over. This function would block until the job is over or an error
 *  occur.
 *
 * Return value:
 *  0, on success.
 *  -EINVAL, if job id is invalid.
*/
int rel_wait(int jid);

/**
 * Description:
 *  Retrieve a job's status, which identified by jid. If the p_job_stat is not NULL, 
 *  the job's current status will be fufilled into *p_job_stat.
 *
 * Return value:
 *  0, on success.
 *  -EINVAL, if job id is invalid.
*/
int rel_stat(int jid, struct rel_stat_st* p_job_stat);

/**
 * Description:
 *  Destory an job by its job descriptor.
 *
 * Return value:
 *  0, on success.
 *  -EINVAL, if job id is invalid.
*/
int rel_destory(int jid);

#endif
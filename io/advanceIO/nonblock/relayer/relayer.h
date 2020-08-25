#ifndef __RELAYER_H__
#define __RELAYER_H__

#include <sys/time.h>
#include <stdint.h>

enum rel_job_stat
{
    REL_JST_RUNNING = 1,
    REL_JST_STOP,
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
    struct timeval start_tm;
    struct timeval end_tm;
};

int rel_init();

/**
 * Description:
 *  Add a new relayer job, and returen the job descriptor if it succeeded.
 *  rel_add() uses two devices' name string, will open it with non-block mode.
 *  if failed, it would return a negtive value of error number.
 *  rel_fadd() is same as rel_add, except using tow opened devices' file descriptors.
 * 
 * Return value:
 *  non-negtive, on success, the relayer job descriptor
 *  -EINVAL, if either device name is invalid.
 *  -ENOMEM, not enough heap memory.
 *  -ENOSPC, the job descriptor table is full.
 *  and other negtive value of error number.
*/
int rel_add(const char* p_dev_name1, const char* p_dev_name2);
int rel_fadd(int fd1, int fd2);


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
 *  -EBUSY, the job has already been canceld.
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
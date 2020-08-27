/**
 * date: 2020年8月22日
 * 有限状态自动机实现两终端直接相互以非阻塞IO方式通信。
 * 该程序需要root权限
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <sys/select.h>

#define TTY1 "/dev/tty9"
#define TTY2 "/dev/tty10"
#define BUFSIZE 1024

#define error_handler_msg(fd, msg)  \
do {\
        if (fd < 0) {\ 
            fprintf(stderr, "%s: %s\n", msg, strerror(errno));\
            exit(EXIT_FAILURE);\
        }\
} while (0);


enum fsm_status
{
    FSM_ST_READ=1,
    FSM_ST_WRITE,
    FSM_ST_EX,
    FSM_ST_TERM
};

struct fsm_st
{
    int fd_src;
    int fd_dst;
    enum fsm_status status;
    int len;
    int off;
    char* errstr;
    char buf[BUFSIZE];
};

static void fsm_driver(struct fsm_st* pfsm)
{
    int byte_written = 0;

    assert(pfsm);
    switch (pfsm->status)
    {
    case FSM_ST_READ:
        pfsm->len = read(pfsm->fd_src, pfsm->buf, sizeof(pfsm->buf));
        if (pfsm->len < 0)
        {
            if (EAGAIN == errno || EINTR == errno)
                ;/* do nonting */
            else
            {   
                pfsm->errstr = "read()";
                pfsm->status = FSM_ST_EX;
            }
            
        }
        else if (pfsm->len > 0)
        {
            pfsm->status = FSM_ST_WRITE;
            pfsm->off = 0;
        }
        else    /* len == 0 */
        {
            /* read over and turn to terminating status*/
            pfsm->status = FSM_ST_TERM;
        }
        
        break;

    case FSM_ST_WRITE:
        byte_written = write(pfsm->fd_dst, pfsm->buf + pfsm->off, pfsm->len);
        if (byte_written < 0)
        {
            if (EAGAIN == errno || EINTR == errno)
                ; /* do nonthing*/
            else
            {
                pfsm->errstr = "write()";
                pfsm->status = FSM_ST_EX;
            }
        }
        else if (byte_written == pfsm->len)
        {
            pfsm->status = FSM_ST_READ;
            pfsm->len = 0;
            pfsm->off = 0;
        }
        else
        {
            /* partially written, status is still write */
            pfsm->len -= byte_written;
            pfsm->off += byte_written;
        }
        
        break;

    case FSM_ST_EX:
        perror(pfsm->errstr);
        pfsm->status = FSM_ST_TERM;
        break;
    
    case FSM_ST_TERM:
        break;

    default:
        abort();
        break;
    }
}

static void relay(const int fd1, const int fd2)
{
	fd_set frdset;
	fd_set fwrset;
	int nfds;	
    int saved_flg1, saved_flg2;
    int err = 0;
	int nfds_ret = 0;
    struct fsm_st fsm1 = {0};
    struct fsm_st fsm2 = {0};

    saved_flg1 = fcntl(fd1, F_GETFL);
    saved_flg2 = fcntl(fd2, F_GETFL);
    if (saved_flg1 < 0 || saved_flg2 < 0)
    {
        perror("fnctl get flags failed.");
        exit(EXIT_FAILURE);
    }

    err = fcntl(fd1, F_SETFL, saved_flg1 | O_NONBLOCK);
    error_handler_msg(err, "fcntl set flag faild.");

    err = fcntl(fd2, F_SETFL, saved_flg2 | O_NONBLOCK);
    error_handler_msg(err, "fcntl set flag faild.");

    fsm1.fd_src = fsm2.fd_dst = fd1;
    fsm1.fd_dst = fsm2.fd_src = fd2;
    fsm1.len = fsm1.off = fsm2.len = fsm2.off = 0;
    fsm1.status = fsm2.status = FSM_ST_READ;
    
	nfds = fd2 + 1;
    while (fsm1.status != FSM_ST_TERM || fsm2.status != FSM_ST_TERM)
    {
		/* init fd_sets */
		FD_ZERO(&frdset);
		FD_SET(fd1, &frdset);
		FD_SET(fd2, &fwrset);
		
		FD_ZERO(&fwrset);
		FD_SET(fd1, &fwrset);
		FD_SET(fd2, &fwrset);
   
   		/* select until any of the tow fd is avaliable. */
   		nfds_ret = select(nfds, &frdset, &fwrset, NULL, NULL);
		if (nfds_ret < 0) 
		{
			perror("select");
			break; // break to exit
		}
  		
		if (FD_ISSET(fsm1.fd_src, &frdset) || FD_ISSET(fsm1.fd_dst, &fwrset))	
   			fsm_driver(&fsm1);
		if (FD_ISSET(fsm2.fd_src, &frdset) || FD_ISSET(fsm2.fd_dst, &fwrset))	
        	fsm_driver(&fsm2);
    }

    fcntl(fd1, F_SETFL, saved_flg1);
    fcntl(fd2, F_SETFL, saved_flg2);
}

int main(int argc, char** argv)
{
    int fd1, fd2;

    fd1 = open(TTY1, O_RDWR | O_NONBLOCK);
    error_handler_msg(fd1, "open()");

    fd2 = open(TTY2, O_RDWR);
    error_handler_msg(fd2, "open()");

    relay(fd1, fd2);
    
    close(fd2);
    close(fd1);

    return 0;
}

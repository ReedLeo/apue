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

#include "relayer.h"

#define TTY1 "/dev/tty11"
#define TTY2 "/dev/tty12"
#define TTY3 "/dev/tty9"
#define TTY4 "/dev/tty10"

#define error_handler_en(res, msg)\
    do {\
        if (res < 0) {\
            fprintf(stderr, msg " fialed with %s\n", strerror(res));\
            exit(EXIT_FAILURE);\
        }\
    } while(0);

int main(int argc, char** argv)
{
    int fd1, fd2, fd3, fd4;
    int res = 0;

    fd1 = open(TTY1, O_RDWR | O_NONBLOCK);
    error_handler_en(fd1, "open()");

    fd2 = open(TTY2, O_RDWR);
    error_handler_en(fd2, "open()");

    res = rel_init();
    error_handler_en(res, "rel_init()");

    res = rel_add(fd1, fd2);
    error_handler_en(res, "rel_fadd()");

    fd3 = open(TTY3, O_RDWR | O_NONBLOCK);
    error_handler_en(fd3, "open()");

    fd4 = open(TTY4, O_RDWR | O_NONBLOCK);
    error_handler_en(fd4, "open()");

    res = rel_add(fd3, fd4);
    error_handler_en(res, "rel_add()");

    while (1)
        pause();

    close(fd2);
    close(fd1);

    return 0;
}
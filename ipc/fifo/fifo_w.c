#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>

#define MODE 0644

static
void err_exit(const char* msg) {
    perror(msg);
    exit(-1);
}

static
void exit_msg(const char* fmt, ...) {
    va_list arg_list;
    va_start(arg_list, fmt);
    vfprintf(stderr, fmt, arg_list);
    va_end(arg_list);
    exit(0);
} 

int main(int argc, char** argv)
{
    int res, fd;
    struct stat st = {0};
    if (argc < 3) {
        fprintf(stderr, "Usage: %s [fifo_name] [msg_to_write]\n", argv[0]);
        exit(-1);
    }
    char* const FIFO_NAME = argv[1];
    char* const MSG = argv[2];
    if ((res = mkfifo(FIFO_NAME, MODE)) < 0) {
        if ((res = stat(FIFO_NAME, &st)) < 0) {
            err_exit("stat");
        }
        if (!S_ISFIFO(st.st_mode)) {
            exit_msg("%s exist but is not a FIFO.", FIFO_NAME);
        }
    }

    if ((fd = open(FIFO_NAME, O_WRONLY, MODE)) < 0) {
        err_exit("open");
    }

    int len = strlen(MSG) + 1;
    if ((res = write(fd, MSG, len)) < 0) {
        err_exit("write");
    }

    close(fd);
    return 0;
}
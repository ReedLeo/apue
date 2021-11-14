#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>

#define MODE 0644
#define BUF_SIZE 1024

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
    ssize_t bytes_read;
    char buf[BUF_SIZE] = {0};
    struct stat st = {0};

    if (argc < 2) {
        fprintf(stderr, "Usage: %s [fifo_name_to_read]\n", argv[0]);
        exit(-1);
    }

    char* const FIFO_NAME = argv[1];

    if ((res = mkfifo(FIFO_NAME, MODE)) < 0) {
        if ((res = stat(FIFO_NAME, &st)) < 0) {
            err_exit("stat");
        }
        if (!S_ISFIFO(st.st_mode)) {
            exit_msg("%s exists but is not a FIFO.", FIFO_NAME);
        }
    }

    if ((fd = open(FIFO_NAME, O_RDONLY, MODE)) < 0) {
        err_exit("open");
    }

    while ((bytes_read = read(fd, buf, sizeof(buf) - 1)) != 0) {
        if (bytes_read < 0) {
            err_exit("read");
        }
        write(1, buf, bytes_read);
    }
 
    close(fd);
    return 0;
}
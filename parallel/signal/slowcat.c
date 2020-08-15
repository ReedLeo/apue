#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


#define CPS 10
#define BUFSIZE CPS


static volatile int loop = 0;

typedef void (*sighandle_t)(int);

void alrm_handler(int arg)
{
    alarm(1);
    loop = 1;   // 产生一个令牌
}

int main(int argc, char** argv)
{
    int fd = -1;
    ssize_t byte_read = 0;
    ssize_t byte_written = 0;
    ssize_t cur_pos = 0;
    char inbuf[BUFSIZE] = {0};

    if (argc < 2)
    {
        fprintf(stderr, "Too few arguments.\n");
        return 1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
        perror("open()");
        return 1;
    }

    signal(SIGALRM, alrm_handler);
    alarm(1);
    while (1)
    {
        // while (!loop)
        //     ;           // 这种空等的方式，白白占用了CPU
        while (!loop)
            pause();       // 然而pause()可以接收任何信号并返回，可能无法准确控制

        loop = 0;   // 消耗一个令牌

        while ((byte_read = read(fd, inbuf, BUFSIZE)) < 0)
        {
            if (EINTR == errno)
            {
                continue;
                puts("read interrupted by signal.");
            }
            else
            {
                perror("read()");
                return 1;
            }
        }
        if (byte_read == 0)
            break;

        byte_written = 0;
        cur_pos = 0;
        while (cur_pos < byte_read)
        {
            byte_written = write(STDOUT_FILENO, inbuf + cur_pos, byte_read - cur_pos);
            cur_pos += byte_written;
        }
    }

    close(fd);
    fd = -1;

    return 0;
}
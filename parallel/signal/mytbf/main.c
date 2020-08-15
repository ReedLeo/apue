#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "mytbf.h"

#define CPS 10
#define BUFSIZE 1024
#define BURST 100


static volatile int token = 0;


int main(int argc, char** argv)
{
    int fd = -1;
    ssize_t byte_read = 0;
    ssize_t byte_written = 0;
    ssize_t cur_pos = 0;
    char inbuf[BUFSIZE] = {0};
    mytbf_t* p_tbf = NULL;
    int token_fetched = 0;

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

    p_tbf = mytbf_init(CPS, BURST);
    if (p_tbf == NULL)
    {
        fprintf(stderr, "init mytbf failed.");
        exit(1);
    }

    while (1)
    {
        token_fetched = mytbf_fetch_token(p_tbf, BUFSIZE);
        if (token_fetched < 0)
        {
            fprintf(stderr, "mytbf_fetch_token(): %s\n", strerror(-token_fetched));
            exit(1);
        }

        while ((byte_read = read(fd, inbuf, token_fetched)) < 0)
        {
            if (EINTR == errno)
            {
                continue;
                fprintf(stderr, "read interrupted by signal.");
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

    mytbf_destory(p_tbf);
    p_tbf = NULL;

    close(fd);
    fd = -1;

    return 0;
}
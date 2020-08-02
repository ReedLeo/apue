#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define BUFSIZE 1024

static char buf[BUFSIZE];

int main(int argc, char** argv)
{
    int fd;
    ssize_t size;
    if (argc < 2)
    {
        fprintf(stderr, "too few arguments.\n");
        return -1;
    }

    while ((fd = open(argv[1], O_RDWR)) < 0)
    {
        if (errno == EINTR)
            continue;
        perror("open()");
        return -1;
    }

    while ((size = read(fd, buf, sizeof(buf))) < 0)
    {
        if (errno == EINTR)
            continue;
        perror("read()");
        return -1;
    }

    


    return 0;
}

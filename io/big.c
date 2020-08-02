#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define FNAME "/tmp/bigfile"

int main()
{
    int fd = -1;
    off_t off = 5LL*1024LL*1024LL*1024LL;
    fd = open(FNAME, O_WRONLY|O_CREAT|O_TRUNC, 0600);
    if (fd < 0)
    {
        perror("open");
        return -1;
    }

    off = lseek(fd, off - 1, SEEK_SET);
    if (off < 0)
    {
        perror("lseek");
        return -1;
    }

    if (write(fd, "", 1) < 0)
    {
        perror("write");
        return -1;
    }

    close(fd);
    return 0;
}
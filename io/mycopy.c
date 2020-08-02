#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// for sake of open()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



int main(int argc, char** argv)
{
    int fd_from = -1;
    int fd_to = -1;
    ssize_t byte_read = 0;
    ssize_t byte_written = 0;
    ssize_t byte_to_write = 0;
    ssize_t write_pos = 0;
    size_t buf_size = 0;
    char* p_buf = NULL;
    int ret_val = 0;

    if (argc < 4)
    {
        fprintf(stderr, "Too few arguments.\n");
        ret_val = 1;
        goto exit_out;
    }

    fd_from = open(argv[1], O_RDONLY);
    fd_to = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd_from < 0 || fd_to < 0)
    {
        perror("open()");
        ret_val = 1;
        goto exit_out;
    }

    buf_size = atoi(argv[3]);
    p_buf = malloc(buf_size);
    if (p_buf == NULL)
    {
        perror("malloc");
        ret_val = 1;
        goto exit_out;
    }
    memset(p_buf, 0, buf_size);

    while (1)
    {
        if ( (byte_read = read(fd_from, p_buf, buf_size)) < 0)
        {
            perror("read");
            ret_val = 1;
            goto exit_out;
        }
        if (byte_read == 0)
        {
            ret_val = 0;
            goto exit_out;
        }

        write_pos = 0;
        byte_to_write = byte_read;
        while ( (byte_written = write(fd_to, p_buf + write_pos, byte_to_write)) < byte_to_write)
        {
            if (byte_written < 0)
            {
                perror("write()");
                goto exit_out;
            }
            byte_to_write -= byte_written;
            write_pos += byte_written;
        }
    }

exit_out:
    free(p_buf);
    close(fd_from);
    close(fd_to);

    return ret_val;
}
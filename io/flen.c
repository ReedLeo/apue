#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    char* p_file = NULL;
    struct stat st = {0};
    if (argc < 2)
    {
        fprintf(stderr, "Too few arguments.\n");
        return 1;
    }

    p_file = argv[1];
    if (stat(p_file, &st) < 0)
    {
        perror("stat()");
        return 1;
    }

    printf("file size: %lld bytes.\n", st.st_size);
    printf("block size: %lld bytes.\n", st.st_blksize);
    printf("blocks: %lld\n", st.st_blocks);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#define PATH_LEN 1024

static off_t path_du(const char* p_path)
{
    static struct stat stat_buf = {0};
    static struct dirent* p_dirent = NULL;
    static struct dirent dent = {0};

    off_t blocks_sum = 0;
    DIR* p_dir = NULL;
    char new_path[PATH_LEN] = {0};

    if (p_path == NULL)
    {
        fprintf(stderr, "p_path is null\n");
        exit(1);
    }

    if (lstat(p_path, &stat_buf) < 0)
    {
        perror("stat()");
        exit(1);
    }

    if (S_ISDIR(stat_buf.st_mode) == 0)
    {
        return stat_buf.st_blocks;
    }

    // is a directory
    blocks_sum = stat_buf.st_blocks;
    // step 1. get all file that not starting with '.'
    p_dir = opendir(p_path);
    if (NULL == p_dir)
    {
        perror("opendir()");
        exit(1);
    }

    while (readdir_r(p_dir, &dent, &p_dirent) == 0 && p_dirent != NULL)
    {
        // exclude .. and .
        if ( strcmp(p_dirent->d_name, ".") && strcmp(p_dirent->d_name, "..") )
        {
            strncpy(new_path, p_path, PATH_LEN);
            strcat(new_path, "/");
            strcat(new_path, p_dirent->d_name);
            blocks_sum += path_du(new_path);
        }
    }

    closedir(p_dir);
    printf("%ld\t%s\n", blocks_sum / 2, p_path);
    return blocks_sum;
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Too few arguments.\n");
        return -1;
    }

    if (argv[1])
        path_du(argv[1]);
    else
    {
        fprintf(stderr, "Arguments rotten\n");
        return -1;
    }

    return 0;
}
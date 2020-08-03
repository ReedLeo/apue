#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <glob.h>

#define PATH_LEN 1024

static off_t path_du(const char* p_path)
{
    static struct stat stat_buf = {0};
    static char new_path[PATH_LEN] = {0};
    static char* p_file_name = NULL;
    off_t blocks_sum = 0;
    glob_t  glob_res = {0};
    

    if (p_path == NULL)
        return 0;
    
    lstat(p_path, &stat_buf);
    if (S_ISDIR(stat_buf.st_mode) == 0)
    {
        return stat_buf.st_blocks;
    }

    // a directory file.
    blocks_sum = stat_buf.st_blocks;
    // get all files' name not starting with '.'
    strncpy(new_path, p_path, PATH_LEN);
    strcat(new_path, "/*" );
    glob(new_path, 0, NULL, &glob_res);

    // get all files' name starting with '.'
    strncpy(new_path, p_path, PATH_LEN);
    strcat(new_path, "/.*");
    glob(new_path, GLOB_APPEND, NULL, &glob_res);

    for (int i = 0; i < glob_res.gl_pathc; ++i)
    {
        p_file_name = strrchr(glob_res.gl_pathv[i], '/');
        if (p_file_name && strcmp(".", p_file_name + 1) && strcmp("..", p_file_name + 1))
        {
            // neither current directory(.) or parent directory(..)
            blocks_sum +=  path_du(glob_res.gl_pathv[i]);
        }
    }

    globfree(&glob_res);
    printf("%ld\t%s\n", blocks_sum / 2, p_path);
    return blocks_sum;
}

int main(int argc, char** argv)
{
    //off_t res;
    if (argc < 2 || argv[1] == NULL)
    {
        fprintf(stderr, "Too few arguments.\n");
        return -1;
    }

    path_du(argv[1]);
    //printf("%ld\t%s\n", res, argv[1]);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#define FN_NAME "."

int main(int argc, char** argv)
{
    int file_count = 0; 
    struct dirent** name_list = NULL;

    if (argc < 2)
    {
        fprintf(stderr, "Too few arguments.\n");
        exit(1);
    }

    file_count = scandir(argv[1], &name_list, NULL, alphasort);
    if (file_count < 0 || name_list == NULL)
    {
        perror("scandir()");
        return 1;
    }
    printf("%d files in %s, there are:\n", file_count, FN_NAME);
    while (file_count--)
    {
        printf("%s\n", name_list[file_count]->d_name);
        free(name_list[file_count]);
    }
    free(name_list);
    
    return 0;
}
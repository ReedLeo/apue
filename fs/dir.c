#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
// for opendir
#include <sys/types.h>
#include <dirent.h>

int main(int argc, char** argv)
{
    DIR* pDir = NULL;
    struct dirent* pDirent = NULL;

    if (argc < 2)
    {
        fprintf(stderr, "Too few arguments.\n");
        return 1;
    }

    pDir = opendir(argv[1]);
    if (pDir == NULL)
    {
        perror("opendir()");
        return 1;
    }

    while (pDirent = readdir(pDir))
    {
        printf("%ld %s\n", pDirent->d_ino, pDirent->d_name);
    }
    

    closedir(pDir);
    return 0;
}

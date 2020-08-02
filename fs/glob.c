#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    char* p_pattern = NULL;
    glob_t globres = {0};

    if (argc < 2)
    {
        fprintf(stderr, "Too few arguments.\n");
        return 1;
    }

    for (int i = 1; i < argc; ++i)
    {
        p_pattern = argv[i];
        if (glob(p_pattern, 0, NULL, &globres))
        {
            perror("glob()");
            break;
        }

        for (int j = 0; j < globres.gl_pathc && globres.gl_pathv[j]; ++j)
        {
            puts(globres.gl_pathv[j]);
        }
    }   

    globfree(&globres);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

extern char** environ;

int main(int argc, char** argv)
{
    char* p_key = NULL;
    char* p_val = NULL;

    if (argc < 3)
    {
        fprintf(stderr, "Too few arguments.\n");
        return 1;
    }

    p_key = argv[1];
    p_val = argv[2];
    assert(p_key && p_val);

    printf("Before setenv(): environ=%p\n", environ);
    for (int i = 0; environ[i]; ++i)
    {
        printf("evn[%d]=%p --> %s\n", i, environ[i], environ[i]);
    }
    getchar();
    
    setenv(p_key, p_val, 1);
    printf("After setenv(): environ=%p\n", environ);
    for (int i = 0; environ[i]; ++i)
    {
        printf("evn[%d] = %p --> %s\n", i,environ[i], environ[i]);
    }

    return 0;
}
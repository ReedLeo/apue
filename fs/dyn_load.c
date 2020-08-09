#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <gnu/lib-names.h>

typedef double (*cos_t)(double);

int main(int argc, char** argv)
{
    void* handle = NULL;
    cos_t p_cos = NULL;
    char* error = NULL;
    printf("pid=%d\nhandle of main: %p\n", getpid(), dlopen(NULL, RTLD_LAZY));
    handle = dlopen(LIBM_SO, RTLD_LAZY);
    if (!handle)
    {
        fprintf(stderr, "%s\n", dlerror());
        return EXIT_FAILURE;
    }
    printf("Loaded handle = %p\n", handle);

    dlerror();  /*  Clear any existing error. */

    p_cos = (cos_t)dlsym(handle, "cos");
    error = dlerror();
    if (error != NULL)
    {
        fprintf(stderr, "%s\n", error);
        exit(EXIT_FAILURE);
    }

    printf("%f\n", p_cos(2.0));
    getchar();
    dlclose(handle);
    return 0;
}
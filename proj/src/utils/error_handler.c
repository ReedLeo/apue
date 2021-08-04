#include "error_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void err_exit(const char* msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    fflush(stderr);
    exit(EXIT_FAILURE);
}
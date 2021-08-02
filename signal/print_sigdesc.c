#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

int main(int argc, char** argv)
{
    printf("signal[%d] description: %s\n", SIGTERM, strsignal(SIGTERM));
    psignal(SIGTERM, "~psignal of SIGTERM~");

    printf("strerror: %s\n", strerror(EINVAL));
    errno = EINVAL;
    perror("~form perror~");
    return 0;
}

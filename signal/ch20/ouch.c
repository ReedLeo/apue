#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

static void sigHandler()
{
    printf("Ouch!\n");
}

int main(int argc, char** argv)
{
    if (signal(SIGINT, sigHandler) == SIG_ERR) {
        perror("signal failed.");
        exit(EXIT_FAILURE);
    }
    
    for (int j = 0; ; ++j) {
        printf("%d\n", j);
        sleep(3);
    }
    return 0;
}

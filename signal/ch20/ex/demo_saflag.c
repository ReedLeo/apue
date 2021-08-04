#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

static int gs_sig_cnt[NSIG] = {0};

static void print_usage(const char* pname)
{
    fprintf(stderr, "Usage: %s [-r|-n] signum\n", pname);
    fprintf(stderr, "\t\t-r, use SA_RESETHAND flag.\n");
    fprintf(stderr, "\t\t-n, use SA_NODEFER flag.\n");
}

static void sig_handler(int signum)
{
    ++gs_sig_cnt[signum];
    printf("recve signal %d (%s) the %d time(s).\n", signum, strsignal(signum), gs_sig_cnt[signum]);
    fflush(stdout);
    sleep(1);
}

int main(int argc, char** argv)
{
    struct sigaction sa = {0};
    int signum = 0;

    if (argc < 3) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    if (!strncmp(argv[1], "-r", 2)) {
        sa.sa_flags |= SA_RESETHAND;
    } else if (!strncmp(argv[1], "-n", 2)) {
        sa.sa_flags |= SA_NODEFER;
    } else {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    
    signum = atoi(argv[2]);
    if (signum < 1 || signum >= NSIG) {
        fprintf(stderr, "Invalid signal number %d\n", signum);
        exit(EXIT_FAILURE);
    }

    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, signum);
    sa.sa_handler = sig_handler;
    sigaction(signum, &sa, NULL);
    
    printf("Now sleeping 15 sec and waiting for signals.\n");
    sleep(15);

    while(1)
        ;

    return 0;
}

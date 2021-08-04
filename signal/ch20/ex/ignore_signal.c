/**
 * This program demonstrates process will never sees(catches) a pending
 * signal if we changed its dispositon to SIG_IGN. 
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

static int gs_sig_cnt[NSIG] = {0};

void sig_handler(int signum)
{
    ++gs_sig_cnt[signum];
}

void print_sig_count(const char* prefix)
{
    int cnt = 0;
    for (int i = 1; i < NSIG; ++i) {
        if (gs_sig_cnt[i]) {
            ++cnt;
            printf("%s: receive signal %d (%s) for %d time(s).\n", prefix, i, strsignal(i), gs_sig_cnt[i]);
        }
    }
    if (!cnt) {
        printf("%s: doesn't receive any signals.\n", prefix);
    }
}

int main(int argc, char** argv)
{
    struct sigaction sa = {0};
    sigset_t full_sigset, block_sigset, pending_sigset, old_sigset;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s [-i(gnore)|-n(ormal)] signum1 signum2 ...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("%s: PID=%ld\n", argv[0], (long)getpid());

    sigfillset(&sa.sa_mask);
    sa.sa_handler = sig_handler;
    for (int i = 1; i < NSIG; ++i) {
        sigaction(i, &sa, NULL);
    }

    sigemptyset(&block_sigset);
    for (int i = 2; i < argc; ++i) {
        int signum = atoi(argv[i]);
        if (signum < 1 || signum >= NSIG) {
            fprintf(stderr, "Invalid signal number at argv[%d]: %s\n", i, argv[i]);
            exit(EXIT_FAILURE);
        }
        if (sigaddset(&block_sigset, signum) < 0) {
            fprintf(stderr, "%s: sigaddset fialed with %s\n", argv[0], strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    sigfillset(&full_sigset);
    if (sigprocmask(SIG_SETMASK, &block_sigset, &old_sigset) < 0) {
        fprintf(stderr, "%s: sigprocmask block fialed with %s\n", argv[0], strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (!strncmp(argv[1], "-i", 2)) {
        sa.sa_handler = SIG_IGN;
        for (int i = 1; i < NSIG; ++i) {
            if (sigismember(&sa.sa_mask, i)) {
                sigaction(i, &sa, NULL);
            }
        }
    }

    printf("%s: sleep for 20 sec.\n", argv[0]);
    sleep(20);

    if (sigprocmask(SIG_SETMASK, &old_sigset, NULL) < 0) {
        fprintf(stderr, "%s: sigprocmask unblock fialed with %s\n", argv[0], strerror(errno));
        exit(EXIT_FAILURE);
    }

    print_sig_count(argv[0]);

    while (1)
        ;
    
    return 0;
}
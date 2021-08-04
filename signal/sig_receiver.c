#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

static int g_sig_int = 0;
static int g_sig_cnt[NSIG] = {0};

void sig_handler(int sig_num)
{
    if (sig_num == SIGINT) {
        g_sig_int = 1;
    } else {
        ++g_sig_cnt[sig_num];
    }
}

void print_sigset(FILE* fp, const sigset_t* p_sigset)
{
    int sig, cnt;
    cnt = 0;
    for (sig = 1; sig < NSIG; ++sig) {
        if (sigismember(p_sigset, sig)) {
            ++cnt;
            fprintf(fp, "\t\t%d (%s)\n", sig, strsignal(sig));
        }
    }

    if (!cnt) {
        fprintf(fp, "\t\t<empty signal set>\n");
    }
}

int main(int argc, char** argv)
{
    struct sigaction sa = {0};
    sigset_t pending_signals = {0};
    sigset_t full_sigset = {0};
    sigset_t old_sigset = {0};

    int sleep_sec = 0;

    printf("%s: PID=%ld is ready to receive signals.\n", argv[0], (long)getpid());

    sigfillset(&sa.sa_mask);
    sa.sa_handler = sig_handler;
    for (int i = 1; i < NSIG; ++i) {
        sigaction(i, &sa, NULL);
    }

    if (argc > 1) {
        sigfillset(&full_sigset);
        sigprocmask(SIG_SETMASK, &full_sigset, &old_sigset);

        sleep_sec = atoi(argv[1]);
        printf("%s: sleeping for %d second(s).\n", argv[0], sleep_sec);
        sleep(sleep_sec);

        sigpending(&pending_signals);
        printf("%s: pending signals are:\n", argv[0]);
        print_sigset(stdout, &pending_signals);

        sigprocmask(SIG_SETMASK, &old_sigset, NULL);
    }
    while (!g_sig_int) {
        continue;
    }

    for (int i = 0; i < NSIG; ++i) {
        if (g_sig_cnt[i])
            printf("%s received %d (%s) for %d time(s).", argv[0], i, strsignal(i), g_sig_cnt[i]);
    }
    return 0;
}
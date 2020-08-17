#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
// for open()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// for openlog, syslog and closelog
#include <syslog.h>

#include <errno.h>

// for sigagction
#include <signal.h>

#define FNAME "/tmp/out"
static FILE* gs_fp;

int daemonize()
{
    pid_t pid;
    int fd; 

    if ((pid = fork()))
        return -1;  // parent;
    

    if ((fd = open("/dev/null", O_RDWR)) < 0)
    {
        // perror("open");
        return -1;
    }

    dup2(fd, 0);
    dup2(fd, 1);
    dup2(fd, 2);

    if (setsid() < 0)
    {
        // perror("setsid");
        return -1;
    }
    
    if (fd > 2)
        close(fd);
    
    chdir("/");
    // umask(0);
    return 0;
}

static void undaemonize(int signum, siginfo_t* si, void* args)
{

    fprintf(gs_fp, "Received a signal: [%d](%s), which is sent by %s"
        , si->si_signo, strsignal(si->si_signo)
        , (si->si_code == SI_KERNEL ? "Kernel" : "Others")
        );
    fflush(gs_fp);
    // signal sent by the kernel.
    if (si->si_code == SI_KERNEL)
    {
        fclose(gs_fp);
        closelog();
        exit(0);
    }
}

int main()
{
    struct sigaction sa = {0};
    
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGTERM);
    sigaddset(&sa.sa_mask, SIGQUIT);
    sa.sa_sigaction = undaemonize;
    sa.sa_flags |= SA_SIGINFO;

    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    openlog("mydaemon", LOG_PID, LOG_DAEMON);
    if (daemonize())
    {
        syslog(LOG_ERR, "daemonize: %s", strerror(errno));
        exit(1);
    }
    
    if ((gs_fp = fopen(FNAME, "w")) == NULL)
    {
        syslog(LOG_ERR, "fopen: %s", strerror(errno));
        exit(1);
    }

    syslog(LOG_INFO, FNAME" was opened.");

    for (int i = 0; ; ++i)
    {
        fprintf(gs_fp, "%d\n", i);
        fflush(gs_fp);
        syslog(LOG_DEBUG, "%d is printed.", i);
        sleep(1);
    }

    return 0;
}
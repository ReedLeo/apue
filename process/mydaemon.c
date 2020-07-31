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

#define FNAME "/tmp/out"

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

int main()
{
    FILE* fp;

    openlog("mydaemon", LOG_PID, LOG_DAEMON);

    if (daemonize())
    {
        syslog(LOG_ERR, "daemonize: %s", strerror(errno));
        exit(1);
    }
    
    if ((fp = fopen(FNAME, "w")) == NULL)
    {
        syslog(LOG_ERR, "fopen: %s", strerror(errno));
        exit(1);
    }

    syslog(LOG_INFO, FNAME" was opened.");

    for (int i = 0; ; ++i)
    {
        fprintf(fp, "%d\n", i);
        fflush(fp);
        syslog(LOG_DEBUG, "%d is printed.", i);
        sleep(1);
    }

    fclose(fp);
    closelog();

    return 0;
}
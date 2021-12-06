#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>

#define PORT 6666
#define BACKLOG 16

#define PERR_EXIT do { \
    perror(NULL);\
    exit(-1);\
} while(0)

#define MAX_EVENT_NUM 2048

int lfd, cfd, efd;

void close_handler(int signum, siginfo_t* psig, void* parg)
{
    close(efd);
    close(lfd);
}

int main(int argc, char** argv)
{
    struct sigaction sa;
    struct epoll_event tep, events[MAX_EVENT_NUM];
    struct sockaddr_in svr_addr = {0};
    struct sockaddr_in clt_addr;
    socklen_t svr_len = sizeof(svr_addr);
    socklen_t clt_len;
    char buf[0x100];

    ssize_t bytes_read;

    lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd < 0) {
        PERR_EXIT;
    }

    svr_addr.sin_family = AF_INET;
    svr_addr.sin_port = htons(PORT);
    svr_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(lfd, (void*)&svr_addr, svr_len) < 0) {
        PERR_EXIT;
    }

    if (listen(lfd, BACKLOG) < 0) {
        PERR_EXIT;
    }

    if ((efd = epoll_create(1)) < 0) {
        PERR_EXIT;
    }

    // set to monitor 
    tep.events = EPOLLIN;
    tep.data.fd = lfd;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, lfd, &tep) < 0)
        PERR_EXIT;
    
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGTERM);
    sigaddset(&sa.sa_mask, SIGINT);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = close_handler;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    while (1) {
        int r = epoll_wait(efd, events, MAX_EVENT_NUM, -1); // wait indefinitely.

        // process r ready events.
        for (int i = 0; i < r; ++i) {
            // only concern the read-ready events.
            if (0 == (events[i].events & EPOLLIN))
                continue;
            
            int sockfd = events[i].data.fd;
            if (sockfd == lfd) {    // a new connection arrived.
                if ((cfd = accept(lfd, (void*)&clt_addr, &clt_len)) < 0) {
                    PERR_EXIT;
                }
                // add the new connection to epoll event RB-tree.
                tep.data.fd = cfd;
                tep.events = EPOLLIN;
                if (epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &tep) < 0)
                    PERR_EXIT;
                continue;
            }

            // there are all read-ready connection.
            if ((bytes_read = read(sockfd, buf, 0x100)) > 0) {
                write(1, buf, bytes_read);
                for (ssize_t i = 0; i < bytes_read; ++i) {
                    buf[i] = toupper(buf[i]);
                }
                write(sockfd, buf, bytes_read);
            } else if (bytes_read == 0) {
                // the remote client disconnected, close this fd and remove it from epoll.
                if (epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL) < 0)
                    PERR_EXIT;
                close(sockfd);
            } else  // bytes_read < 0, on error.
                PERR_EXIT;
        }
    }

    return 0;
}
/**
 * @file pipe1.c
 * @author your name (you@domain.com)
 * @brief Demonstrate the usage of pipe by emulating 'ls | wc -l;. 
 *  The parent process will execute 'ls' and write its output to 
 *  the pipe's write end, then the child process execute 'wc -l'
 *  to read pipe's read end as its stdin. 
 * @version 0.1
 * @date 2021-11-13
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <error.h>
#include <stdlib.h>
#include <sys/types.h>

int main(int argc, char** argv)
{
    int pd[2] = {0};
    int res = 0;
    pid_t pid;

    if ((res = pipe(pd)) < 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    if ((pid = fork()) == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {   // parent
        // dup pipe's write end to stdout.
        dup2(pd[1], 1);
        // close pipe's read end
        close(pd[0]);
        execlp("ls", "ls", NULL);
        fprintf(stderr, "exec ls fialed.\n");
    } else {    // child
        // dup pipe's read end to stdin.
        dup2(pd[0], 0);
        // close pipe's write end.
        close(pd[1]);
        execlp("wc", "wc", "-l", NULL);
        fprintf(stderr, "exec wc -l failed.\n");
    }
    return 0;
}
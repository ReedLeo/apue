#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char** argv) {
    pid_t pid[2] = {0};
    int pd[2] = {0};
    int res = 0;

    if ((res = pipe(pd)) < 0) {
        perror("pipe failed.\n");
        exit(-1);
    }

    for (int i = 0; i < 2; ++i) {
        pid[i] = fork();
        if (pid[i] < 0) {
            fprintf(2, "[%d] fork failed.\n", i, strerror(errno));
            exit(-1);
        } else if (pid[i] == 0) {
            if (i) {
                close(pd[1]);
                dup2(pd[0], 0);
                execlp("wc", "wc", "-l", NULL);
                fprintf(2, "exec wc -l failed.\n");
            } else {
                close(pd[0]);
                dup2(pd[1], 1);
                execlp("ls", "ls", NULL);
                fprintf(2, "exec ls failed.\n");
            }
        }
    }

    // It's neccessary to close pipe's write end in the parent, because
    // the child who execute 'wc -l' will read from pipe's read end until
    // it return EOF. If any write ends of this pipe are still open, the 
    // child will loop forever for no EOF it can read.
    // In this case, it's no need to close pipe's read end in the parent.
    close(pd[0]); 
    close(pd[1]);
    // wait for 2 child processes to exit.
    wait(NULL);
    wait(NULL);
    return 0;
}
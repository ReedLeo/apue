#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main()
{
    pid_t pid = 0;

    printf("[%d]: Begin!\n", getpid());
    
    // fflush(stdout); // should be invoke before fork()

    if (pid = fork())
    {
        printf("[%d]: Parent working\n", getpid());
    }
    else
    {
        printf("[%d]: Chlid working!\n", getpid());
    }
    printf("[%d]: End!\n", getpid());

    getchar();
    return 0;
}
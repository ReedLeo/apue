#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM 200
#define LEFT 30000000
#define RIGHT (LEFT + NUM + 1)

int main()
{
    pid_t pid;
    int i, j, mark;

    for (i = LEFT; i < RIGHT; ++i)
    {   
        if (pid = fork())
            continue;
        mark = 0;
        for (j = 2; j < i/2; ++j)
        {   
            if (i % j == 0)
            {
                mark = 1;
                break;
            }
        }
        if (mark == 0)
            printf("%d\n", i);
        exit(0);
    }
    for (i = LEFT; i < RIGHT; ++i) {
        wait(NULL);
    }
    return 0;
}

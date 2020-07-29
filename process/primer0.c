#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define NUM 200
#define LEFT 30000000
#define RIGHT (LEFT + NUM + 1)

int main()
{
    int i, j, mark;
    for (i = LEFT; i < RIGHT; ++i)
    {
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
    }
    return 0;
}
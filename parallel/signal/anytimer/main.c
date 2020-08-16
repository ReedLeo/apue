#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "anytimer.h"

#define TASK_NUM 10

#define DEF_HANDLER(i) \
static void handler_##i(int signum) \
{ \
    printf("task[%d]: %s is running.\n", i, __FUNCTION__);\
}

#define HANDLER(i) handler_##i

#define ADD_TASK(i) anytimer_add_task(HANDLER(i), i+1, 1)

DEF_HANDLER(0);
DEF_HANDLER(1);
DEF_HANDLER(2);
DEF_HANDLER(3);
DEF_HANDLER(4);
DEF_HANDLER(5);
DEF_HANDLER(6);
DEF_HANDLER(7);
DEF_HANDLER(8);
DEF_HANDLER(9);

int main(int argc, char** argv)
{
    anytask_t* tasks[TASK_NUM] = {0};
    anytimer_handler_t handlers_arr[TASK_NUM] = {
        HANDLER(0),
        HANDLER(1),
        HANDLER(2),
        HANDLER(3),
        HANDLER(4),
        HANDLER(5),
        HANDLER(6),
        HANDLER(7),
        HANDLER(8),
        HANDLER(9)
    };

    if (anytimer_init())
    {
        fprintf(stderr, "anytimer_init() failed.\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < TASK_NUM; ++i)
    {
        tasks[i] = anytimer_add_task(handlers_arr[i], i+1, 1);
        if (tasks[i] == NULL)
        {
            fprintf(stderr, "anytimer_add_task() failed.\n");
            exit(EXIT_FAILURE);
        }
    }

    getchar();
    
    anytimer_destory();
}
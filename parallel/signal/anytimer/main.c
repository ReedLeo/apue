#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "anytimer.h"

#define TASK_NUM 5

#define DEF_HANDLER(i) \
static void handler_##i(void* args) \
{ \
    printf("task[%d]: %s is running.\n", i, __FUNCTION__);\
}

#define HANDLER(i) handler_##i

#define ADD_TASK(i) at_add_task(HANDLER(i), i+1, 1)

DEF_HANDLER(0);
DEF_HANDLER(1);
DEF_HANDLER(2);
DEF_HANDLER(3);
DEF_HANDLER(4);
// DEF_HANDLER(5);
// DEF_HANDLER(6);
// DEF_HANDLER(7);
// DEF_HANDLER(8);
// DEF_HANDLER(9);

int main(int argc, char** argv)
{
    int td_arr[TASK_NUM] = {0};
    int ret = 0;
    at_handler_t handlers_arr[TASK_NUM] = {
        HANDLER(0),
        HANDLER(1),
        HANDLER(2),
        HANDLER(3),
        HANDLER(4),
        // HANDLER(5),
        // HANDLER(6),
        // HANDLER(7),
        // HANDLER(8),
        // HANDLER(9)
    };
    puts("Begin!!");

    if (at_init())
    {
        fprintf(stderr, "at_init() failed.\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < TASK_NUM; ++i)
    {
        td_arr[i] = at_add_task(handlers_arr[i], NULL, i+1, 1);
        if (td_arr[i] < 0)
        {
            fprintf(stderr, "at_add_task() failed.\n");
            exit(EXIT_FAILURE);
        }
    }

    getchar();
    
    for (int i = 0; i < TASK_NUM; ++i)
    {    
        if ((ret = at_wait_task(td_arr[i]))< 0)
        {
            fprintf(stderr, "at_wait_task failed with %s\n", strerror(-ret));
            exit(EXIT_FAILURE);
        }
    }
    at_destory();
    puts("End!!");
    return 0;
}
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>


#define TRNUM 4
#define LEFT 30000000
#define RIGHT ((LEFT) + 201)

static int gs_num;
static pthread_mutex_t gs_mut_num = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gs_cond_num_consumed = PTHREAD_COND_INITIALIZER;
static pthread_cond_t gs_cond_num_produced = PTHREAD_COND_INITIALIZER;

void* primer_calc_proc(void* arg)
{
    int n;
    int flag = 0;
    while (1)
    {
        pthread_mutex_lock(&gs_mut_num);
        while (gs_num == 0)
        {   
            pthread_cond_wait(&gs_cond_num_produced, &gs_mut_num);
        }
        if (gs_num == -1)
        {
            pthread_mutex_unlock(&gs_mut_num);
            break;
        }
        n = gs_num;
        gs_num = 0;
        pthread_cond_signal(&gs_cond_num_consumed);
        pthread_mutex_unlock(&gs_mut_num);

        flag = 0;
        for (int i = 2; i < n/2; ++i)
        {
            if (n % i == 0)
            {
                flag = 1;
                break;
            }
        }
        if (!flag)
            printf("thread[%d] get a primer: %d\n", (int)arg, n);
    }
    pthread_exit(NULL);
}

int main(int argc, char** argv)
{
    pthread_t tids[TRNUM] = {0};
    puts("Begin!!");
    
    pthread_mutex_lock(&gs_mut_num);
    for (int i = 0; i < TRNUM; ++i)
    {
        pthread_create(tids+i, NULL, primer_calc_proc, (void*)i);
    }
    pthread_mutex_unlock(&gs_mut_num);
    
    for (int i = LEFT; i < RIGHT; ++i)
    {
        pthread_mutex_lock(&gs_mut_num);
        while (gs_num != 0)
        {
            pthread_cond_wait(&gs_cond_num_consumed, &gs_mut_num);
        }
        gs_num = i;
        pthread_cond_signal(&gs_cond_num_produced);
        pthread_mutex_unlock(&gs_mut_num);
    }

    pthread_mutex_lock(&gs_mut_num);
    while (gs_num != 0)
    {
        pthread_cond_wait(&gs_cond_num_consumed, &gs_mut_num);
    }
    gs_num = -1;
    pthread_cond_broadcast(&gs_cond_num_produced);
    pthread_mutex_unlock(&gs_mut_num);

    for (int i = 0; i < TRNUM; ++i)
    {
        pthread_join(tids[i], NULL);
    }

    puts("End!!");
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

void int_handler(int arg)
{
    write(1, "!", 1);
    return;
}

int main()
{
    signal(SIGINT, int_handler);
    for (int i = 0; i < 10; ++i)
    {
        write(1, "*", 1);
        sleep(1);
    }
    return 0;
}
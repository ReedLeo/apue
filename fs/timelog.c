#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define FN_NAME "/tmp/out"
#define BUFSIZE 1024

int main(int argc, char** argv)
{
    FILE* pf = NULL;
    char buf[BUFSIZE];
    int count = 0;
    time_t tim;
    struct tm* p_tm = NULL;

    if ((pf = fopen(FN_NAME, "a+")) == NULL)
    {
        perror("fopen()");
        return 1;
    }

    while (fgets(buf, BUFSIZE, pf))
    {
        ++count;
    }

    while (1)
    {
        if (time(&tim) == (time_t)-1)
        {
            perror("time()");
            return 2;
        }
        if ((p_tm = localtime(&tim)) == NULL)
        {
            perror("localtime()");
            return 3;
        }
        strftime(buf, BUFSIZE, "%Y-%m-%d %H:%M:%S", p_tm);
        fprintf(pf, "%-4d %s\n", ++count, buf);
        fflush(pf);
        sleep(1);
    }

    fclose(pf);
    return 0;
}
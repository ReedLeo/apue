#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

extern char* optarg;
extern int optind, opterr, optopt;

#define OPT_STR "-y:mdH:MS"
#define FMT_STR_SIZE 1024
#define BUFSIZE 1024

#define APPEND_FMT(pattern) strncat(fmtstr, pattern, FMT_STR_SIZE - strlen(fmtstr))

int main(int argc, char** argv)
{
    static char fmtstr[FMT_STR_SIZE] = { 0 };
    static char buf[BUFSIZE] = {0};
    int ch = 0;
    time_t tmt;
    FILE* fp = stdout;

    while (1)
    {
        ch = getopt(argc, argv, OPT_STR);
        if (ch < 0 || ch == '?' || ch == ':')
            break;

        switch (ch)
        {
        case 1:
            if (fp == stdout)
            {
                puts(optarg);
                fp = fopen(optarg, "w");
                if (fp == NULL)
                {
                    perror("fopen()");
                    fp = stdout;
                }
            }
            break;

        case 'y':
            if (strcmp(optarg, "2") == 0)
            {
                APPEND_FMT("%y ");
            }
            else if (strcmp(optarg, "4") == 0)
            {
                APPEND_FMT("%Y ");
            }
            else
            {
                fprintf(stderr, "Invalid argument of -y");
            }
            break;

        case 'm':
            APPEND_FMT("%m ");
            break;

        case 'd':
            APPEND_FMT("%d ");
            break;

        case 'H':
            if (strcmp(optarg, "12") == 0)
            {
                APPEND_FMT("%I(%p) ");
            }
            else if (strcmp(optarg, "24") == 0)
            {
                APPEND_FMT("%H ");
            }
            else
            {
                fprintf(stderr, "Invalid argument of -H");
            }
            
            break;

        case 'M':
            APPEND_FMT("%M ");
            break;

        case 'S':
            APPEND_FMT("%S ");
            break;

        default:
            break;
        }
    }

    time(&tmt);
    strftime(buf, sizeof(buf), fmtstr, localtime(&tmt));
    strncat(buf, "\n", sizeof(buf) - strlen(buf));
    fputs(buf, fp);
    return 0;
}
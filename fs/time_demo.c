#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define BUFSIZE 1024

int main(int argc, char** argv)
{
    time_t tmt;
    struct tm st_tm = {0};
    struct tm* p_st_tm = NULL;
    char* p_fmtstr = NULL;
    char buf[BUFSIZE] = {0};

    tmt = time(NULL);
    if (tmt == (time_t)-1)
    {
        perror("time()");
        return 1;
    }

    printf("[1] the raw time_t of now: %ld\n", tmt);

    p_fmtstr = ctime(&tmt);
    if (p_fmtstr == NULL)
    {
        perror("ctime()");
        return 1;
    }
    printf("[2] ctime() of now: %s", p_fmtstr);


    p_st_tm = localtime(&tmt);
    if (p_st_tm == NULL)
    {
        perror("localtime()");
        return 1;
    }
    p_fmtstr = asctime(p_st_tm);
    if (p_fmtstr == NULL)
    {
        perror("asctime()");
        return 1;
    }
    printf("[3] asctime(localtime(t)) = %s", p_fmtstr);

    memcpy(&st_tm, p_st_tm, sizeof(*p_st_tm));
    st_tm.tm_mday += 30;
    st_tm.tm_isdst = 0;
    tmt = mktime(&st_tm);
    printf("[4] mktime(): %ld\n", tmt);

    p_st_tm = gmtime(&tmt);
    if (p_st_tm == NULL)
    {
        perror("gmtime()");
        return 1;
    }
    printf("[5] gmtime(): %d-%d-%d %02d:%02d:%02d\n"
            , p_st_tm->tm_year+1900
            , p_st_tm->tm_mon+1
            , p_st_tm->tm_mday
            , p_st_tm->tm_hour
            , p_st_tm->tm_min
            , p_st_tm->tm_sec);

    strftime(buf, sizeof(buf), "%F %T", p_st_tm);
    printf("[6] strftime(): %s\n", buf);

    return 0;
}
#include <string.h>
#include <stdio.h>

void parse(char* buf)
{
    char* tok = NULL;
    while ((tok = strsep(&buf, " \t")) != NULL)
    {
        if (tok[0] == '\0')
            continue;
        puts(tok);
    }
}

int main()
{
    char* buf = NULL;
    size_t n = 0;
    if (getline(&buf, &n, stdin) >= 0)
    {
        parse(buf);
    }
    
    free(buf);
    return 0;
}
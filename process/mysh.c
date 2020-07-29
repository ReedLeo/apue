#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/wait.h>

#define DELIM " \t\n"

typedef struct cmd_t
{
    glob_t glob_res;
} cmd_t;

void parse(char**, cmd_t*);
void prompt(void);

int main()
{
    char* buf = NULL;
    pid_t pid;
    ssize_t read = 0;
    size_t n = 0;
    cmd_t cmd = {0};

    while (1)
    {
        
        prompt();

        read = getline(&buf, &n, stdin);
        if (read < 0)
        {
            perror("geline()");
            free(buf);
            exit(1);
        }

        parse(&buf, &cmd);

        free(buf);
        if (0)  // 内部命令
        {}
        else    // 外部命令
        {
            if (pid = fork())
            {
                globfree(&cmd.glob_res);
                wait(NULL);
            }
            else
            {
                execvp(cmd.glob_res.gl_pathv[0], cmd.glob_res.gl_pathv);
                perror("execvp");
                exit(1);
            }
        }
    }
    return 0;
}

void prompt()
{
    printf("local@myshell$ ");
}

void parse(char** p_str, cmd_t* p_cmd)
{
    char* tok = NULL;
    int i = 0;
    while ((tok = strsep(p_str, DELIM)) != NULL)
    {
        if (tok[0] == '\0')
            continue;
        
        glob(tok, GLOB_NOCHECK | GLOB_APPEND * i, NULL, &p_cmd->glob_res);
        i = 1;
    }
    return;
}
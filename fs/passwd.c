#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <shadow.h>
#include <crypt.h>

int main(int argc, char** argv)
{
    char* p_name = NULL;
    char* p_pwd = NULL;
    char* p_encrypted_pwd = NULL;
    struct spwd* p_spwd = NULL;

    if (argc < 2)
    {
        fprintf(stderr, "Too few arguments.\n");
        return 1;
    }

    p_name = argv[1];
    p_pwd = getpass("Password: ");
    if (p_pwd == NULL)
    {
        perror("getpass()");
        return 1;
    }

    p_spwd = getspnam(p_name);
    if (p_spwd == NULL)
    {
        perror("getspnam()");
        return 1;
    }

    p_encrypted_pwd = crypt(p_pwd, p_spwd->sp_pwdp);
    if (p_encrypted_pwd == NULL)
    {
        perror("crypt()");
        return 1;
    }

    if (strcmp(p_encrypted_pwd, p_spwd->sp_pwdp) == 0)
    {
        puts("Login Success!");
    }
    else
    {
        puts("Account or Password error.");
    }
    
    return 0;
}
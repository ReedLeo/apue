#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main()
{
    printf("Before setuid: uid=%d euid=%d\n", getuid(), geteuid());
    getchar();
    setuid(0);
    printf("After setuid: uid=%d euid=%d\n", getuid(), geteuid());
    return 0;
}
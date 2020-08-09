#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#define FN_BEGIN() printf("%s: Begin()\n", __FUNCTION__);
#define FN_END() printf("%s: End()\n", __FUNCTION__);

static jmp_buf jb;

static void c(void)
{
    FN_BEGIN();
    longjmp(jb, 1);
    FN_END();
}

static void b(void)
{
    FN_BEGIN();
    c();
    FN_END();
}

static void a(void)
{
    FN_BEGIN();
    b();
    FN_END();
}

int main(int argc, char** argv)
{
    
    FN_BEGIN();
    if (setjmp(jb))
    {
        puts("return from longjmp()");
    }
    else
    {
        a();
        puts("normal return");
    }
    FN_END();
    return 0;
}
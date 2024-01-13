#include <stdio.h>

#ifndef NL
#define NL "\n"
#endif

int main(int argc, const char * argv[]) {
    (void)argc;
    (void)argv;

    printf("sev!!!"NL);
    asm volatile ("sev;");
    printf("done"NL);
    return 0;
}


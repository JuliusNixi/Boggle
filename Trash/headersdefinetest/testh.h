
#include <stdio.h>
#ifdef SERVER
    extern void g(void);
#endif



void f(void) {

    printf("F.\n");
    #ifdef SERVER
        g();
    #endif

}


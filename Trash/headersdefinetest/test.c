#define SERVER
#include "testh.h"

// Used to see if i can from common.c call
// (only if I am compiling the server) the gameEndQueue() function
// implemented in server.c when compiling.

// Using a #define SERVER

void g(void){

    printf("G.\n");

}

int main(void){

    printf("MAIN.\n");
    f();
    return 0;
}



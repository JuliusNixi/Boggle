#include <stdio.h>

// A test on innested structs.

// Sub struct.
struct sub {

    // Value.
    int v;

};

// Main struct.
struct m{

    // Sub struct.
    struct sub wp;

};


void modifysub(struct sub* x) {

    x->v = 100;
    return;

}

void p(struct m* x) {

    modifysub(&(x->wp));
    return;

}


int main(void) {

    struct m ms;
    ms.wp.v = 2;
    // Expected 2.
    printf("Before p: %d.\n", ms.wp.v);
    p(&ms);
    // Expected 100.
    printf("After p: %d.\n", ms.wp.v);
    return 0;

}
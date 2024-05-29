// Sub struct.
struct j {
    int v;
};

// Main struct.
#include <stdio.h>
struct s{
    struct j wp;
};
struct s i;

void modify(struct j* x) {

    x->v = 100;
    return;

}

void p(struct s* x) {

    modify(&x->wp);
    return;

}


int main(void) {

    i.wp.v = 2;
    printf("before p: %d\n", i.wp.v);
    p(&i);
    printf("then p: %d\n", i.wp.v);
    return 0;

}
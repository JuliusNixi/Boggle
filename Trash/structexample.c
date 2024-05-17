struct j {
    int v;
};

// Main struct
#include <stdio.h>
struct s{
    struct j wp;
};
struct s i;


void modifica(struct j* x) {
    x->v = 100;
}

void p(struct s* x) {

    modifica(&x->wp);

}


int main(void) {

    i.wp.v = 2;
    printf("prima v: %d\n", i.wp.v);
    p(&i);
    printf("dopo v: %d\n", i.wp.v);
    return 0;


}
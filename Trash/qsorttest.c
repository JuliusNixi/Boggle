
#include <stdio.h>


struct Test {
    int v;
};

int compar(const void* a, const void* b){

   struct Test* x = (struct Test*) a;
   struct Test* y = (struct Test*) b;
   return (x->v) - (y->v);

}


int main(void) {

    struct Test array[5];
    for (unsigned int i = 0; i < 5; i++) array[i].v = 5 - i;
    printf("BEFORE\n");
    for (unsigned int i = 0; i < 5; i++) printf("V: %d.\n", array[i].v);
    qsort(array, 5, sizeof(struct Test), compar);
    printf("AFTER\n");
    for (unsigned int i = 0; i < 5; i++) printf("V: %d.\n", array[i].v);

    return 0;

}



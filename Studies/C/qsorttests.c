
#include <stdio.h>

// Tests on qsort to sort an array of structs.


struct Test {
    int v;
};

int compar(const void* a, const void* b){

   struct Test* x = (struct Test*) a;
   struct Test* y = (struct Test*) b;
   return (x->v) - (y->v);

}

int main(void) {

    #define N 5
    struct Test array[N];

    // 5 4 3 2 1
    // Initializing struct array field.
    for (unsigned int i = 0; i < N; i++) array[i].v = N - i;

    printf("BEFORE QSORT.\n");
    for (unsigned int i = 0; i < N; i++) printf("V: %d.\n", array[i].v);

    qsort(array, N, sizeof(struct Test), compar);

    // Should be sorted the array of struct now.
    printf("AFTER QSORT.\n");
    for (unsigned int i = 0; i < N; i++) printf("V: %d.\n", array[i].v);

    return 0;

}



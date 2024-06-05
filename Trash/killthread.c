#include <stdio.h>
#include <pthread.h>

pthread_t t;
void* f(void* a) {

    printf("Hello from the thread!\n");
    fflush(stdout);
    pthread_cancel(t);
    fflush(stdout);
    printf("Am i alive?\n");
    fflush(stdout);

    return NULL;
}
int main(void) {

    
    int retvalue = pthread_create(&t, NULL, f, NULL);
    printf("Hello from MAIN!\n");
    fflush(stdout);


    return 0;
}

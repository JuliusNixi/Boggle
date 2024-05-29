#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_t test;

void* w(void* args) {

    printf("ID 1: %lu.\n", (unsigned long) test);
    printf("ID 2: %lu.\n", (unsigned long) pthread_self());
    fflush(stdout);

    return NULL;

}

int main(void) {

    
    int retvalue = pthread_create(&test, NULL, w, NULL);
    sleep(1);

    return 0;

}



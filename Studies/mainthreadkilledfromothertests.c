
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

pthread_t maint;

// Test on killing main from an another thread.

void* f(void* args) {

    sleep(8);
    printf("Killing main...\n");
    fflush(stdout);
    pthread_cancel(maint);

    sleep(10);
    // This should be not executed.
    // Instead I was wrong and it is executed even with the dead main, unbelievable!
    printf("I am still alive also without main.\n");
    fflush(stdout);

    return NULL;

}

void atExit(void) {

    printf("Exiting...\n");
    fflush(stdout);
    return;

}

int main(void) {

    // Registering exit function to see if its called when another thread kill this one (main).
    atexit(atExit);

    maint = pthread_self();

    // Creating the other thread.
    pthread_t t;
    pthread_create(&t, NULL, f, NULL);

    while (1){
        printf("Hello from main!\n");
        fflush(stdout);
        sleep(2);
    }
    
    return 0;

}




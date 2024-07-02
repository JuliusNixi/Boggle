#include <stdio.h>
#include <pthread.h>

// Tests on threads IDs.

pthread_t t;
pthread_t maint;

void* f(void* args) {

    // Should be equals.
    printf("ID FROM var T: %lu.\n", (unsigned long) t);
    printf("ID FROM pthread_self() T: %lu.\n", (unsigned long) pthread_self());
        
    printf("ID FROM MAIN: %lu.\n", (unsigned long) maint);

    return NULL;

}

int main(void) {

    maint = pthread_self();

    // Creating and waiting a thread.
    pthread_create(&t, NULL, f, NULL);
    pthread_join(t, NULL);

    return 0;

}



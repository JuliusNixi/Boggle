#include <stdio.h>
#include <pthread.h>

// Tests on threads (and threads IDs).

pthread_t t;
pthread_t maint;

void* f(void* args) {

    // Should be equals.
    printf("ID FROM T: %lu.\n", (unsigned long) t);
    printf("ID FROM pthread_self() (T): %lu.\n", (unsigned long) pthread_self());
        
    printf("ID FROM MAIN: %lu.\n", (unsigned long) maint);

    fflush(stdout);

    return NULL;

}

int main(void) {

    maint = pthread_self();
    pthread_create(&t, NULL, f, NULL);
    pthread_join(t, NULL);

    return 0;

}



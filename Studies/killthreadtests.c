#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

// Tests with pthread_cancel() to kill a thread.

pthread_t t;
pthread_t t2;
void* f(void* a) {

    printf("Hello from the first thread!\n");
    fflush(stdout);

    pthread_cancel(t2);
    pthread_cancel(t);

    // Sometimes, without this sleep, this thread print also the next printf, before being cancelled.
    sleep(5);

    // This WON'T be printed (most likely).
    printf("Am i alive (first thread)?\n");
    fflush(stdout);

    return NULL;

}

void* f2(void* a) {

    printf("Hello from the second thread!\n");
    fflush(stdout);

    // Sometimes, this thread print also the next printf, before being cancelled.

    // This COULD be printed or NOT.
    printf("Am i alive (second thread)?\n");
    fflush(stdout);

    return NULL;

}

int main(void) {

    
    printf("Hello from MAIN!\n");
    fflush(stdout);

    int retvalue = pthread_create(&t, NULL, f, NULL);
    retvalue = pthread_create(&t2, NULL, f2, NULL);

    pthread_join(t, NULL);

    return 0;

}

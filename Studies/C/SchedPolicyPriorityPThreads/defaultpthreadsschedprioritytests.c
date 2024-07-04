#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#define N 100

// This tests file is the continuation of the "./changepthreadschedprioritytests.c".
// We create many "normal" threads and one "special" thread.
// All of these will have the same scheduling policy and priority as pthreads.
// Then all the threads will try to get a mutex, print a message and release it.
// The purpose is to see what the default behaviour (default pthreads' scheduling policy and 
// priority) leads to. 
// And whether if it's possible for the "normal" threads (which obviously are many more) 
// to be executed many times at the expense of the "special" thread, this would be a problem
// in the project's game.

pthread_mutex_t m;
pthread_t SpecialThread;

void* threadFunction(void* args) {

    while(1) {
        pthread_mutex_lock(&m);

        printf("I'm %lu.", (unsigned long) pthread_self());
        if (pthread_self() == SpecialThread) printf(" SPECIAL!\n");
        else printf("\n");
        fflush(stdout);

        pthread_mutex_unlock(&m);
    }

    return NULL;

}

int main() {

    pthread_t LowerThreads[N];

    pthread_mutex_init(&m, NULL);

    // Using for both lower and high priority threads the default scheduling policy with the
    // default priority.

    // Creating and starting N pthreads.
    for (unsigned int i = 0U; i < N; i++) {
        if (pthread_create(&(LowerThreads[i]), NULL, threadFunction, NULL) != 0) {
            perror("Failed to create pthread");
            exit(EXIT_FAILURE);
        }
    }

    // Create and start the pthread.
    if (pthread_create(&SpecialThread, NULL, threadFunction, NULL) != 0) {
        perror("Failed to create pthread");
        exit(EXIT_FAILURE);
    }

    // Waiting forever.
    while(1) sleep(1);

    return 0;

}
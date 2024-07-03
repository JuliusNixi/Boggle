#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#define N 100

pthread_mutex_t m;
pthread_t SpecialThread;


void* thread_function(void* arg) {

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

    pthread_t LowerThread[N];

    pthread_mutex_init(&m, NULL);

    for (int i = 0; i < N; i++) {
        if (pthread_create(&(LowerThread[i]), NULL, thread_function, NULL) != 0) {
            perror("Failed to create thread");
            exit(1);
        }
    }

    // Create and start the thread
    if (pthread_create(&SpecialThread, NULL, thread_function, NULL) != 0) {
        perror("Failed to create thread");
        exit(1);
    }

    while(1) sleep(1);

    return 0;

}
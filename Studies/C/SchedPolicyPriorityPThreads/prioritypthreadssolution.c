#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define NUM_LOW_PRIORITY_THREADS 100

// This file contains the tests that led to the solution (used in the project) 
// of the problem that we could not solve with all the others files in this folder.
// We simply use an additional mutex and a boolena (char) var that will be responsible
// for dividing the threads into priority and nonpriority.
// In this case, because the prority threads must execute different code from the non-priority 
// threads, the use of two separate functions is necessary.

pthread_mutex_t main_mutex;
pthread_mutex_t priority_mutex;
char special_thread_waiting = 0;

void* low_priority_thread_f(void* args) {

    while (1) {

        pthread_mutex_lock(&priority_mutex);
        pthread_mutex_lock(&main_mutex);
        pthread_mutex_unlock(&priority_mutex);

        printf("Low priority thread %lu acquired the mutex.\n", (unsigned long) pthread_self());
        // Some work...

        pthread_mutex_unlock(&main_mutex);

        // Sleep to permits other threads to acquire the mutex.
        sleep(1);

    }

    return NULL;

}

void* special_thread_f(void* args) {

    while (1) {

        pthread_mutex_lock(&priority_mutex);
        special_thread_waiting = 1;
        printf("Special thread is waiting for the mutex.\n");

        pthread_mutex_lock(&main_mutex);
        special_thread_waiting = 0;
        pthread_mutex_unlock(&priority_mutex);

        printf("Special thread acquired the mutex.\n");
        // Some work...

        pthread_mutex_unlock(&main_mutex);

        // Sleep to permits other threads to acquire the mutex.
        // With this sleep so high seems to not works but it's not so!
        // It works! Because if we remove or decrease this sleep, this thread will
        // always re-acquire the mutex at the expense of all other lesser priorities.
        sleep(1);

    }

    return NULL;

}

int main() {

    pthread_t low_priority_threads[NUM_LOW_PRIORITY_THREADS];
    pthread_t special_thread;

    pthread_mutex_init(&main_mutex, NULL);
    pthread_mutex_init(&priority_mutex, NULL);

    // Create N low priority pthreads.
    for (unsigned int i = 0U; i < NUM_LOW_PRIORITY_THREADS; i++)
        pthread_create(&low_priority_threads[i], NULL, low_priority_thread_f, NULL);

    // Create "special" high priority pthread.
    pthread_create(&special_thread, NULL, special_thread_f, NULL);

    // Waiting forever.
    while (1) sleep(1);

    return 0;

}
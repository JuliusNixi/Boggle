#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_LOW_PRIORITY_THREADS 100

pthread_mutex_t main_mutex;
pthread_mutex_t priority_mutex;
int special_thread_waiting = 0;

void* low_priority_thread(void* arg) {
    int id = *(int*)arg;
    while (1) {
        pthread_mutex_lock(&priority_mutex);
        pthread_mutex_lock(&main_mutex);
        pthread_mutex_unlock(&priority_mutex);

        printf("Low priority thread %d acquired the mutex\n", id);
        pthread_mutex_unlock(&main_mutex);
        sleep(1);  // Give other threads a chance
    }
    return NULL;
}

void* special_thread(void* arg) {
    while (1) {
        pthread_mutex_lock(&priority_mutex);
        special_thread_waiting = 1;
        printf("Special thread is waiting for the mutex\n");

        pthread_mutex_lock(&main_mutex);
        special_thread_waiting = 0;
        pthread_mutex_unlock(&priority_mutex);

        printf("Special thread acquired the mutex\n");
        pthread_mutex_unlock(&main_mutex);

  usleep(50);
      //  sleep(1);  // Wait before trying again
    }
    return NULL;
}

int main() {
    pthread_t low_priority_threads[NUM_LOW_PRIORITY_THREADS];
    pthread_t special_thread_id;
    int thread_ids[NUM_LOW_PRIORITY_THREADS];

    pthread_mutex_init(&main_mutex, NULL);
    pthread_mutex_init(&priority_mutex, NULL);

    // Create low priority threads
    for (int i = 0; i < NUM_LOW_PRIORITY_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&low_priority_threads[i], NULL, low_priority_thread, &thread_ids[i]);
    }

    // Create special high priority thread
    pthread_create(&special_thread_id, NULL, special_thread, NULL);

    // Wait for threads to finish (which they never will in this example)
    pthread_join(special_thread_id, NULL);
    for (int i = 0; i < NUM_LOW_PRIORITY_THREADS; i++) {
        pthread_join(low_priority_threads[i], NULL);
    }

    pthread_mutex_destroy(&main_mutex);
    pthread_mutex_destroy(&priority_mutex);

    return 0;
}
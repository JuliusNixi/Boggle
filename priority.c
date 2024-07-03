#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#define N 100

pthread_mutex_t m;
pthread_t SpecialThread;

// Run as root on Linux.


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

    pthread_attr_t SpecialThreadAttr;
    pthread_attr_t LowerThreadAttr;

    struct sched_param SpecialThreadParam;
    struct sched_param LowerThreadParam;

    int policy = SCHED_RR;
    int max_priority, min_priority;

    pthread_mutex_init(&m, NULL);

    // Initialize thread attributes
    pthread_attr_init(&SpecialThreadAttr);
    pthread_attr_init(&LowerThreadAttr);

    // Set the scheduling policy to SCHED_RR
    if (pthread_attr_setschedpolicy(&SpecialThreadAttr, policy) != 0) {
        perror("Failed to set scheduling policy");
        exit(1);
    }
    if (pthread_attr_setschedpolicy(&LowerThreadAttr, policy) != 0) {
        perror("Failed to set scheduling policy");
        exit(1);
    }

    pthread_attr_setinheritsched(&SpecialThreadAttr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&LowerThreadAttr, PTHREAD_EXPLICIT_SCHED);

    // Get max and min priorities for SCHED_RR
    max_priority = sched_get_priority_max(policy);
    min_priority = sched_get_priority_min(policy);

    if (max_priority == -1 || min_priority == -1) {
        perror("Failed to get priority range");
        exit(1);
    }

    // Set thread priority to max
    SpecialThreadParam.sched_priority = max_priority;
    if (pthread_attr_setschedparam(&SpecialThreadAttr, &SpecialThreadParam) != 0) {
        perror("Failed to set thread priority");
        exit(1);
    }
    LowerThreadParam.sched_priority = min_priority;
    if (pthread_attr_setschedparam(&LowerThreadAttr, &LowerThreadParam) != 0) {
        perror("Failed to set thread priority");
        exit(1);
    }

    for (int i = 0; i < N; i++) {
        if (pthread_create(&(LowerThread[i]), &LowerThreadAttr, thread_function, NULL) != 0) {
            perror("Failed to create thread");
            exit(1);
        }
    }

    // Create and start the thread
    if (pthread_create(&SpecialThread, &SpecialThreadAttr, thread_function, NULL) != 0) {
        perror("Failed to create thread");
        exit(1);
    }

    while(1) sleep(1);

    return 0;

}
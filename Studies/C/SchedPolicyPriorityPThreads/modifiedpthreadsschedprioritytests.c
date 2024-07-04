#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#define N 100

// Run as root on Linux.
// Otherwise we obtain the following error: "Failed to create pthread: Success".

// This tests file is the continuation of the "./changepthreadschedprioritytests.c".
// We create many "normal" threads and one "special" thread.
// The "special" thread will have the maximum scheduling priority.
// All the "normal" threads will have the minimum scheduling priority.
// Then all the threads will try to get a mutex, print a message and release it.
// The purpose is to see what result we will obtain, hoping an important difference from the
// result obtained in the "./defaultpthreadsschedprioritytests.c".
// Hopefully it will be evident in the result how the "special" thread acquires the mutex
// many more times than the "normal" threads despite the fact that the latter are many more.

// SPOILER: It doesn't work... :(
// In the end, the solution adopted in the project is in "./prioritypthreadssolution.c".

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

    pthread_attr_t SpecialThreadAttr;
    pthread_attr_t LowerThreadAttr;

    struct sched_param SpecialThreadParam;
    struct sched_param LowerThreadParam;

    int policy = SCHED_RR;
    int max_priority, min_priority;

    pthread_mutex_init(&m, NULL);

    // Initialize pthread attributes.
    pthread_attr_init(&SpecialThreadAttr);
    pthread_attr_init(&LowerThreadAttr);

    // Set the scheduling policy to SCHED_RR.
    if (pthread_attr_setschedpolicy(&SpecialThreadAttr, policy) != 0) {
        perror("Failed to set scheduling policy");
        exit(EXIT_FAILURE);
    }
    if (pthread_attr_setschedpolicy(&LowerThreadAttr, policy) != 0) {
        perror("Failed to set scheduling policy");
        exit(EXIT_FAILURE);
    }

    // NEEDED, OTHERWISE DOESN'T WORK!
    // https://stackoverflow.com/questions/69951151/scheduling-policy-and-priority-using-pthread-does-not-make-any-difference
    pthread_attr_setinheritsched(&SpecialThreadAttr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&LowerThreadAttr, PTHREAD_EXPLICIT_SCHED);

    // Get max and min priorities for SCHED_RR.
    max_priority = sched_get_priority_max(policy);
    min_priority = sched_get_priority_min(policy);

    if (max_priority == -1 || min_priority == -1) {
        perror("Failed to get priority range");
        exit(EXIT_FAILURE);
    }

    // Set pthread priority to max.
    SpecialThreadParam.sched_priority = max_priority;
    if (pthread_attr_setschedparam(&SpecialThreadAttr, &SpecialThreadParam) != 0) {
        perror("Failed to set pthread priority");
        exit(EXIT_FAILURE);
    }
    // Set pthread priority to min.
    LowerThreadParam.sched_priority = min_priority;
    if (pthread_attr_setschedparam(&LowerThreadAttr, &LowerThreadParam) != 0) {
        perror("Failed to set pthread priority");
        exit(EXIT_FAILURE);
    }

    // Creating and starting N pthreads.
    for (unsigned int i = 0U; i < N; i++) {
        if (pthread_create(&(LowerThreads[i]), &LowerThreadAttr, threadFunction, NULL) != 0) {
            perror("Failed to create pthread");
            exit(EXIT_FAILURE);
        }
    }

    // Create and start the "special" pthread.
    if (pthread_create(&SpecialThread, &SpecialThreadAttr, threadFunction, NULL) != 0) {
        perror("Failed to create pthread");
        exit(EXIT_FAILURE);
    }
    
    // Waiting forever.
    while(1) sleep(1);

    return 0;

}
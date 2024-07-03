#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

// Run as root on Linux.

void* thread_function(void* arg) {
    int policy;
    struct sched_param param;
    
    // Print Hello World
    printf("Hello World!\n");
    fflush(stdout);

    // Get the current thread's scheduling policy
    if (pthread_getschedparam(pthread_self(), &policy, &param) != 0) {
        perror("Failed to get thread scheduling parameters");
        return NULL;
    }

    // Print the scheduling policy
    printf("Thread scheduling policy: ");
    switch(policy) {
        case SCHED_FIFO:
            printf("SCHED_FIFO\n");
            break;
        case SCHED_RR:
            printf("SCHED_RR\n");
            break;
        case SCHED_OTHER:
            printf("SCHED_OTHER\n");
            break;
        default:
            printf("Unknown\n");
    }

    // Print the thread priority
    printf("Thread priority: %d\n", param.sched_priority);
    fflush(stdout);

    return NULL;
}

void myClear(void){
    printf("PRESS ENTER TO PROCEED.\n");
    char buffer[100];
    read(STDIN_FILENO, buffer, 100);
    system("clear");
}

int main() {

    pthread_t SpecialThread;
    pthread_t LowerThread;

    pthread_attr_t SpecialThreadAttr;
    pthread_attr_t LowerThreadAttr;

    struct sched_param SpecialThreadParam;
    struct sched_param LowerThreadParam;

    int policy = SCHED_RR;
    int max_priority, min_priority;

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

    printf("SCHED_RR priority range: %d (min) to %d (max)\n", min_priority, max_priority);

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

    myClear();

    // Create and start the thread
    if (pthread_create(&SpecialThread, &SpecialThreadAttr, thread_function, NULL) != 0) {
        perror("Failed to create thread");
        exit(1);
    }
    // Wait for the thread to finish
    pthread_join(SpecialThread, NULL);

    myClear();

    if (pthread_create(&LowerThread, &LowerThreadAttr, thread_function, NULL) != 0) {
        perror("Failed to create thread");
        exit(1);
    }
    // Wait for the thread to finish
    pthread_join(LowerThread, NULL);

    // Clean up
    pthread_attr_destroy(&SpecialThreadAttr);
    pthread_attr_destroy(&LowerThreadAttr);

    return 0;

}
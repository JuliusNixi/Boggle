#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

// Run as root on Linux.
// Otherwise we obtain the following error: "Failed to create pthread: Success".

// This file contains tests to see if it's possible to change the scheduling policy and ESPECIALLY
// the priority of a pthread. Specifically we will create one pthread with the highest priority 
// (SpecialThread) and one with the lowest (LowerThread).

// All these tests have been made for experiment with threads priorities.
// But they were not used in the project.

void* threadFunction(void* args) {

    int policy;
    struct sched_param param;

    // Get the current pthread's scheduling policy.
    if (pthread_getschedparam(pthread_self(), &policy, &param) != 0) {
        perror("Failed to get pthread scheduling parameters");
        return NULL;
    }

    // Print the scheduling policy.
    printf("Thread scheduling policy: ");
    switch(policy) {
        case SCHED_FIFO:
            printf("SCHED_FIFO.\n");
            break;
        case SCHED_RR:
            printf("SCHED_RR.\n");
            break;
        case SCHED_OTHER:
            printf("SCHED_OTHER.\n");
            break;
        default:
            printf("Unknown.\n");
    }

    // Print the pthread priority.
    printf("Thread priority: %d.\n", param.sched_priority);
    fflush(stdout);

    return NULL;

}

void myClear(void){

    printf("PRESS ENTER TO PROCEED...\n");
    char buffer[100];
    read(STDIN_FILENO, buffer, 100);
    system("clear");

}

int main() {

    system("clear");

    pthread_t SpecialThread;
    pthread_t LowerThread;

    pthread_attr_t SpecialThreadAttr;
    pthread_attr_t LowerThreadAttr;

    struct sched_param SpecialThreadParam;
    struct sched_param LowerThreadParam;

    int policy = SCHED_RR;
    int max_priority, min_priority;

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

    printf("SCHED_RR priority range: from %d (min) to %d (max).\n", min_priority, max_priority);

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

    myClear();

    // Create and start the pthread.
    if (pthread_create(&SpecialThread, &SpecialThreadAttr, threadFunction, NULL) != 0) {
        perror("Failed to create pthread");
        exit(EXIT_FAILURE);
    }
    // Wait for the pthread to finish.
    pthread_join(SpecialThread, NULL);

    myClear();

    // Create and start the pthread.
    if (pthread_create(&LowerThread, &LowerThreadAttr, threadFunction, NULL) != 0) {
        perror("Failed to create pthread");
        exit(EXIT_FAILURE);
    }
    // Wait for the pthread to finish.
    pthread_join(LowerThread, NULL);

    // Clean up.
    pthread_attr_destroy(&SpecialThreadAttr);
    pthread_attr_destroy(&LowerThreadAttr);

    return 0;

}
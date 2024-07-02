#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

// This file contains tests on pthread signals.

pthread_t t2t;
pthread_t t3t;

pthread_t maint;

// Set this to 0 to execute the first test, to 1 to execute the second test.
char test1ortest2 = 1; 

void* f(void* args) {

    if (test1ortest2 == 0) {
        printf("Cancelling the main thread. The program should anyway continue.\n");
        printf("Also with pthread_exit() executed in the main thread we will obtain the same behaviour.\n");
        pthread_cancel(maint);
    }else {
        printf("After 10 seconds the main thread will exit and all the program will die.\n");
    }

    sleep(5);
    printf("Sending signals SIGUSR1 to the two threads...\n");
    pthread_kill(t2t, SIGUSR1);
    pthread_kill(t3t, SIGUSR1);
    pthread_exit(NULL);
    return NULL;

}

void* t2(void* args) {

    printf("I'm T2 thread ID: %lu.\n", (unsigned long) pthread_self());
    while (1) sleep(1);
    return NULL;

}

void* t3(void* args) {

    printf("I'm T3 thread ID: %lu.\n", (unsigned long) pthread_self());
    while (1) sleep(1);
    return NULL;

}

void h(int signum) {

    while(1){
        printf("I'm the SIGUSR1 handler, thread ID: %lu.\n", (unsigned long) pthread_self());
        sleep(1);
    }
    return;

}

int main(void) {

    maint = pthread_self();

    // Registering SIGUSR1 signal handler.
    struct sigaction sigusr1;
    sigusr1.sa_flags = 0;
    sigusr1.sa_handler = h;  
    sigaction(SIGUSR1, &sigusr1, NULL);

    // Signals sender thread.
    pthread_t t;
    pthread_create(&t, NULL, f, NULL);

    // Signals receivers threads.
    pthread_create(&t2t, NULL, t2, NULL);
    pthread_create(&t3t, NULL, t3, NULL);

    sleep(10);
    printf("Main thread exiting...\n");

    return 0;

}



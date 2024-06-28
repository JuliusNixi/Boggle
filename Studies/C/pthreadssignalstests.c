#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

// This file contains tests on pthread signals.

pthread_t t2t;
pthread_t t3t;

void* f(void* args) {

    sleep(3);
    pthread_kill(t2t, SIGUSR1);
    pthread_kill(t3t, SIGUSR1);
    return NULL;

}

void* t2(void* args) {

    printf("I'm T2: %lu.\n", (unsigned long int)pthread_self());
    fflush(stdout);
    while (1) {
        sleep(1);
    }
    return NULL;

}

void* t3(void* args) {

    printf("I'm T3: %lu.\n", (unsigned long int)pthread_self());
    fflush(stdout);
    while (1) {
        sleep(1);
    }
    return NULL;

}

void h(int signum) {

    while(1){
        fflush(stdout);
        printf("I'm H: %lu.\n", (unsigned long int)pthread_self());
        fflush(stdout);
        sleep(1);
    }
    return;

}

int main(void) {

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

    sleep(8);

    return 0;

}



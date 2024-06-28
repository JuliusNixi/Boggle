#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

pthread_t maint;
pthread_t t;

// Tests on killing main from an another thread.

// To use test1 set 0, to use test2 set 1.
char test1ortest2 = 0;

void* f(void* args) {

    if (test1ortest2 == 0) {
        printf("Killing main with pthread_cancel()...\n");
        fflush(stdout);
        pthread_cancel(maint);
    }

    sleep(10);
    // This should be not executed with test1.
    // Instead I was wrong and it is executed even with the dead main, unbelievable!
    int c = 0;
    while (1){
        printf("I am still alive also without main.\n");
        fflush(stdout);
        sleep(1);
        if (c++ == 5) break;
    }

    if (test1ortest2) {
        printf("Killing main with signal, should stop all the program (also this thread now)...\n");
        pthread_kill(maint, SIGUSR2);
    }

    while(1) sleep(1);

    return NULL;

}

void atExit(void) {

    printf("Exiting...\n");
    fflush(stdout);
    return;

}

void sigUSR2Handler(int signum) {

    char str[] = "Bye bye...\n";
    write(STDOUT_FILENO, str, strlen(str));
    exit(EXIT_FAILURE);

}


int main(void) {

    if (test1ortest2) {
        struct sigaction sigusr2; 
        sigusr2.sa_flags = 0;
        sigusr2.sa_handler = sigUSR2Handler;   
        sigaction(SIGUSR2, &sigusr2, NULL);
    }

    // Registering exit function to see if its called when another thread kill this one (main).
    atexit(atExit);

    maint = pthread_self();

    // Creating the other thread.
    pthread_create(&t, NULL, f, NULL);

    while (1){
        printf("Hello from main!\n");
        fflush(stdout);
        sleep(2);
    }
    
    return 0;


}









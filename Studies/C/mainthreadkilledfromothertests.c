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
    
    printf("Hello from the other thread, ID: %lu.\n", (unsigned long) pthread_self());

    if (test1ortest2 == 0) {
        printf("Killing main with pthread_cancel()...\n");
        fflush(stdout);
        pthread_cancel(maint);
        sleep(5);
        // This should be not executed with test1 because i have killed the main.
        // Instead, I was wrong and it's executed even with the dead main, unbelievable!
        int c = 0;
        while (1){
            printf("I am still alive also without main.\n");
            fflush(stdout);
            sleep(1);
            if (c++ == 5){
                printf("The other thread is exiting...\n");
                pthread_exit(NULL);
            } 
        }
    }

    if (test1ortest2 == 1) {
        printf("Killing main with signal, should stop all the program (also this thread now)...\n");
        pthread_kill(maint, SIGUSR2);
        while(1){
            printf("Hello from the other thread!\n");
            sleep(1);
        } 
    }

    return NULL;

}

void atExit(void) {

    // IMPORTANT: This function could be called by any thread!
    // If the main thread is dead, when the other thread exit, it's him to call this function,
    // not the main thread.
    printf("I'm the atExit() function. Caller thread ID: %lu.\n", (unsigned long) pthread_self());
    fflush(stdout);
    return;

}

void sigUSR2Handler(int signum) {

    char str[] = "Bye bye, exiting from SIGUSR2, handled by the main thread...\n";
    write(STDOUT_FILENO, str, strlen(str));
    exit(EXIT_FAILURE);

}


int main(void) {

    if (test1ortest2) {
        printf("SIGUSR2 handler registered succesfully!\n");
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
        printf("Hello from main, thread ID: %lu.\n", (unsigned long) pthread_self());
        fflush(stdout);
        sleep(1);
    }
    
    return 0;


}









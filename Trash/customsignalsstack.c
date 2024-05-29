#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>


void sigintHandler(int signum) {

    struct sigaction sigint;
    stack_t ss;

    // Getting previously setted stack and handler structure.
    sigaction(SIGINT, NULL, &sigint);
    sigaltstack(NULL, &ss);

    // Initialize i and p. 
    int i = -100;
    void* p = ss.ss_sp;
    printf("P HANDLER: %p.\n", p);

    // Getting the value previously written.
    void* p2 = p + MINSIGSTKSZ;
    int* pi = (int*) p2;
    printf("VALUE HANDLER: %d.\n", *pi);
    
    return;

}


int main(void) {

    // In this test i want to create a custom shared stack to which the signal handler 
    // and the main will have access.
    // The end goal is to initialize an integer value in the main and make it possible
    // to read and write it from the signal handler function.

    // IMPORTANT: At the end i used a different implementation without sharing stack.

    struct sigaction sigint; // SIGINT signal handler.

    stack_t ss;

    // Allocating the right size of meory on the heap.
    // MINSIGSTKSZ is the minimum stack size for the signal handler.
    size_t s = MINSIGSTKSZ + sizeof(int);
    void* p = (void*) malloc(s);
    if (p == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }     
    printf("P MAIN: %p.\n", p);

    // Writing the int.
    void* p2 = p + MINSIGSTKSZ;
    int* pi = (int*) p2;
    *pi = 42;
    printf("VALUE MAIN: %d.\n", *pi);

    ss.ss_sp = p;
    ss.ss_size = s;
    ss.ss_flags = 0;
    
    // Enabling the stack.
    if (sigaltstack(&ss, NULL) == -1) {
        perror("sigaltstack");
        exit(EXIT_FAILURE);
    }

    // Enabling the signal handler.
    sigint.sa_flags = SA_ONSTACK;
    sigint.sa_handler = sigintHandler;      /* Address of a signal handler */
    sigemptyset(&sigint.sa_mask);
    if (sigaction(SIGINT, &sigint, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }           


    printf("SIGINT signal handler registered correctly.\n");


    while (1) sleep(1);
 

    return 0;

}



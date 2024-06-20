#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

// This file contains some tests on modifying (in a secure way)
// a global var from a signal handler.

volatile sig_atomic_t* s;

pthread_t t;
pthread_t t2;
void* f(void* args) {

    sleep(3);
    printf("T: %lu. Sending SIGUSR1 signal to T2.\n", (unsigned long int)pthread_self());
    fflush(stdout);
    pthread_kill(t2, SIGUSR1);

    return NULL;

}

void* f2(void* args) {

    while(1) sleep(1);

    return NULL;

}

void handler(int sig){
    
    void* getv = (void*) s;
    char* fs = (char*) getv;
    fflush(stdout);
    printf("H: %s", fs);
    fflush(stdout);
    fs[4] = '\0';
    return;

}

int main(void) {

    struct sigaction sigusr1;
    sigusr1.sa_flags = SA_SIGINFO;
    sigusr1.sa_handler = handler;  
    sigaction(SIGUSR1, &sigusr1, NULL);

    printf("sizeof(void*): %lu. sizeof(s): %lu.\n", (unsigned long)sizeof(void*), (unsigned long)sizeof(s));
    fflush(stdout);

    char tmpstr[] = "Hello World!\n";
    int l = strlen(tmpstr) + 1;
    s = malloc(sizeof(char)* l);
    char* xs = (char*) s;
    strcpy(xs, tmpstr);
    xs[l - 1] = '\0';
    printf("M: %s", xs);
    fflush(stdout);

    pthread_create(&t, NULL, f, NULL);
    pthread_create(&t2, NULL, f2, NULL);
 
    sleep(6);
    // If all works, the string should be cut to "Hell".
    printf("M: %s\n", xs);
    fflush(stdout);
 
    return 0;

}



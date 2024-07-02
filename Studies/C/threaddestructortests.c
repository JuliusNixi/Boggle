#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// This file contains the tests to create a thread destructor function, called before
// the thread exits to cleanup some data if necessary.
// Moreover, this mechanism permits to a thread to save its specific "key", an 
// object that can be used to save thread specific data.

// A simplier alternative previous considered as thread destructor was:
// void pthread_cleanup_push(void (*routine)(void *), void *arg);
// void pthread_cleanup_pop(int execute);
// But these functions, require to be called in pairs, with the thread code
// (one at the beginning and one at the end of thread code) and the thread exit must
// mandatorily be in the middle, so they are very limiting.

// At the end not implemented in the project because not needed.

static pthread_key_t key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

pthread_t t, t2, maint;

// Destructor of all threads. Called before destruction of a registered thread.
void threadsDestructor(void* args) {

    // Can be used args to know wich thread is calling this function.
    printf("Destructor called for the thread %c. With ID: %lu.\n", *((char*)args), (unsigned long) pthread_self());
    // Remember to free the allocated memory to avoid leaks!
    free(args);

    return;
  
}

// Executed only once (from the first calling thread).
void makeKey(void){

    printf("makeKey(). Executed only once. I am the thread (ID): %lu.\n", (unsigned long) pthread_self());
    pthread_key_create(&key, threadsDestructor);
    return;

}

// This function MUST be executed by all threads to register each thread destructor.
void registerDestructor(void) {

    void* ptr = NULL;

    // Executed only once (from the first calling thread).
    pthread_once(&key_once, makeKey);

    printf("registerDestructor(). I am the thread (ID): %lu.\n", (unsigned long) pthread_self());

    char data;
    pthread_t current = pthread_self();
    if (current == maint) data = 'M';
    else if (current == t) data = 'T';
    else if (current == t2) data = 'R';

    if ((ptr = pthread_getspecific(key)) == NULL) {
        ptr = malloc(sizeof(char));
        *((char*)(ptr)) = data;
        printf("Registered data from thread (ID): %lu.\n", (unsigned long) pthread_self());
        pthread_setspecific(key, ptr);
    }

    // Retrieve thread specific data (not necessary in this test).
    // char* dataptr = pthread_getspecific(key)
    ;

    return;
    
}

// First thread function.
void* f(void* args){

    t = pthread_self();
    printf("Hello from t (thread ID): %lu.\n", (unsigned long) t);

    registerDestructor();

    printf("T thread exiting...\n");

    return NULL;

}

// Second thread function.
void* f2(void* args) {

    printf("Hello from t2 (thread ID): %lu.\n", (unsigned long) t2);

    registerDestructor();

    printf("T2 thread exiting...\n");

    return NULL;

}

void atExit(void){

    printf("I am the atExit() function. Called by thread (should be the main) (ID): %lu.\n", (unsigned long) pthread_self());
    return;

}

int main(void) {

    atexit(atExit);
    printf("atExit() registered.\n");

    maint = pthread_self();
    printf("Hello from main (ID): %lu.\n", (unsigned long) maint);

    // Creating two threads.    
    pthread_create(&t, NULL, f, NULL);
    pthread_create(&t2, NULL, f2, NULL);

    // Remember to register the thread destructor also for the main thread!
    registerDestructor();

    // Waiting other threads to terminate.
    pthread_join(t, NULL);
    pthread_join(t2, NULL);

    printf("Main exiting...\n");

    // Important, without this it's called directly the atExit() and not the thread destructor
    // for the main.
    pthread_exit(NULL);

    return 0;

}



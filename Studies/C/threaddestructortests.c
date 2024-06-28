#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// This file contains the tests to create a thread destructor function, called before
// the thread exits to cleanup.
// Moreover, this mechanism permits to a thread to save its specific "key", an 
// object that can be used to save thread specific data.

// A simplier alternative previous considered was:
// void pthread_cleanup_push(void (*routine)(void *), void *arg);
// void pthread_cleanup_pop(int execute);
// But these functions, require to be called in pairs, with the thread code
// (one at the beginning and one at the end of thread code) and the thread exit must
// mandatorily be in the middle, so they are very limiting.

static pthread_key_t key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

pthread_t t, t2, maint;

// Destructor of all threads.
void localThreadsDestructor(void* args) {

    // Can be used args to know wich thread is calling this function.
    printf("Destructor called for the thread %c. With ID: %lu.\n", *((char*)args), (unsigned long)pthread_self());
    fflush(stdout);
    free(args);

    return;
  
}

// Executed only once (from the first calling thread).
void makeKey(void){

    printf("makeKey(). Executed only once. I am the thread (ID): %lu.\n", (unsigned long) pthread_self());
    fflush(stdout);
    pthread_key_create(&key, localThreadsDestructor);
    return;

}

// This function MUST be executed by all threads to register the thread destructor.
void registerDestructor(void) {

    void* ptr = NULL;

    // Executed only once (from the first calling thread).
    pthread_once(&key_once, makeKey);

    printf("registerDestructor(). I am the thread (ID): %lu.\n", (unsigned long) pthread_self());
    fflush(stdout);

    char data;
    pthread_t current = pthread_self();
    if (current == maint) data = 'M';
    else if (current == t) data = 'T';
    else if (current == t2) data = 'R'; // T2 == R.

    // This if is executed only the first time (by each thread) to setup the data.
    if ((ptr = pthread_getspecific(key)) == NULL) {
        ptr = malloc(sizeof(char));
        *((char*)(ptr)) = data;
        printf("I am executed only one from each thread (ID): %lu.\n", (unsigned long) pthread_self());
        fflush(stdout);
        // ... Other things that may be necessary carried out ONLY the FIRST TIME. ...
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
    printf("Hello from t (ID): %lu.\n", (unsigned long) t);
    fflush(stdout);

    registerDestructor();

    printf("T thread exiting...\n");
    fflush(stdout);

    return NULL;

}

// Second thread function.
void* f2(void* args) {

    t2 = pthread_self();
    printf("Hello from t2 (ID): %lu.\n", (unsigned long) t2);
    fflush(stdout);

    registerDestructor();

    printf("T2 thread exiting...\n");
    fflush(stdout);

    return NULL;

}

// atexit() function.
void atExit(void){

    printf("I am the atExit() function. Called by thread (should be the main) (ID): %lu.\n", (unsigned long) pthread_self());
    fflush(stdout);
    return;

}

int main(void) {

    atexit(atExit);
    printf("atExit() registered.\n");

    maint = pthread_self();
    printf("Hello from main (ID): %lu.\n", (unsigned long) maint);

    fflush(stdout);
    
    pthread_create(&t, NULL, f, NULL);
    pthread_create(&t2, NULL, f2, NULL);

    registerDestructor();

    pthread_join(t, NULL);
    pthread_join(t2, NULL);

    printf("Main exiting...\n");
    fflush(stdout);

    // Important, without this it's called directly the atExit() and not the thread destructor
    // for the main.
    pthread_exit(NULL);

    return 0;

}



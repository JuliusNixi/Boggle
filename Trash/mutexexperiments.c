/*

    THESE ARE EXPERIMENT ON RACE CONDITIONS, THREADS AND MUTEX
    TO BETTER UNDERSTAND WHAT HAPPENS AND DO NOT CAUSE TROUBLE IN THE PROJECT,
    I WILL TRY TO FLIP A BIT READING/WRITING IT FROM MAIN AND CONCURRENTLY 
    FROM A DEDICATED SIGINT THREAD.

*/


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

sigset_t   signal_mask;  /* signals to block         */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int secret = 0;

void* signal_thread(void* arg)
{
    int       sig_caught;    /* signal caught       */
    int       rc;            /* returned code       */

    while (1){
        rc = sigwait(&signal_mask, &sig_caught);
        pthread_mutex_lock(&mutex);  // -> ADDED TO SOLVE RACE CONDITION 1
        printf("THREAD: %d.\n", secret);
        if (rc != 0) {
            /* handle error */
            printf("Error in sigwait().\n");
        }
        if (secret == 0) secret = 1;
        else secret = 0;
        pthread_mutex_unlock(&mutex);  // -> ADDED TO SOLVE RACE CONDITION 1
    }

    return NULL;

}


int main(void) {

    pthread_t  sig_thr_id;      /* signal handler thread ID */
    int        rc;              /* return code              */

    // Creating a mask, that will block the SIGINT signal, and enabling
    // it for the current main thread.
    sigemptyset (&signal_mask);
    sigaddset (&signal_mask, SIGINT);
    rc = pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
    if (rc != 0) {
        /* handle error */
        printf("Error in setting the pthread mask.\n");
        exit(EXIT_FAILURE);
        
    }

    /* any newly created threads INHERIT the signal mask */

    // Creating the thread that will handle the SIGINT blocked signal (and eventually others).
    // Is COUNTERINTUITIVE that the handler has the SIGNINT blocked, when
    // is the same one that should handle it.
    // But this is the implementation of the sigwait() and how it works.
    // From manual:
    /*
    [...] The signals specified by set should be blocked, but not ignored, at the
    time of the call to sigwait(). [...]
    */
    rc = pthread_create (&sig_thr_id, NULL, signal_thread, NULL);
    if (rc != 0) {
        // handle error 
        printf("Error in creating the pthread signal handler.\n");
        exit(EXIT_FAILURE);  
    }
    /* APPLICATION CODE */
    while(1) {
        pthread_mutex_lock(&mutex);  // -> ADDED TO SOLVE RACE CONDITION 1
        printf("THREAD MAIN: %d.\n", secret);
        // sleep(5); // -> USE WITH NORMAL - EXPECTED
        // The sleep is to "artificially force" the right behaviour.
        // Normally I would like to have the behaviuor with the sleep above (and without the below RACE CONDITION 1).
        //------------------------------------------------------------
        if (secret == 0) secret = 1;
        else secret = 0;
        //------------------------------------------------------------
        sleep(5); // -> RACE CONDITION 1
        // The sleep is to "artificially force" the race condition.
        // I WOULDN'T like to have the behaviour with the sleep above.
        pthread_mutex_unlock(&mutex); // -> ADDED TO SOLVE RACE CONDITION 1
        sleep(1);   // ALWAYS BE CAREFUL WHEN LOCKING/UNLOCKING A MUTEX IN A WHILE!
        // WITHOUT THIS LAST SLEEP, THE OTHER THREAD WILL NEVER ACQUIRE THE MUTEX (TESTED)!
    }

    return 0;

}

/*

///////////////////////////        WITHOUT MUTEX        ///////////////////////////         

NORMAL - EXPECTED:

THREAD MAIN     THREAD SIGNAL       SECRET VALUE MEMORY
      R                                       0
      F                                       1
                      R                       1
                      F                       0

OUTPUT
----------------
THREAD MAIN: 0.
THREAD: 1.
----------------

POSSIBLE RACE CONDITION 1:

THREAD MAIN     THREAD SIGNAL       SECRET VALUE MEMORY
      R                                       0
                      R                       0
      F                                       1
                      F                       0


OUTPUT
----------------
THREAD MAIN: 0.
THREAD: 0.
----------------

*/

// Let's try to use pthread_mutex_t to solve the issue (forcing the NORMAL - EXPECTED behaviour)... 



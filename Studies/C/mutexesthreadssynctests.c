/*

    These are tests on race conditions, threads and mutexes
    to better understand what happens and do not cause troubles in the project,
    I will try to flip a bit reading/writing it from main and concurrently 
    from a dedicated SIGINT thread handler.

    After the start press CTRL + C to send the SIGINT signal.

*/


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

// Signals to block.
sigset_t signal_mask;
// WARNING: Only the static mutexes could be initialized in this way!
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int secret = 0;

void* signalsThread(void* args){

    // Sig number caught. Should be 2 == SIGINT.
    int signum;

    while (1){
        sigwait(&signal_mask, &signum);

        if (signum != SIGINT) {
            printf("Unrecognized signal, can handle only SIGINT.\n");
            exit(EXIT_FAILURE);
        }

        pthread_mutex_lock(&mutex);  // -> ADDED TO SOLVE RACE CONDITION 1
        printf("THREAD: %d.\n", secret);
        // Flipping secret.
        if (secret == 0) secret = 1;
        else secret = 0;
        pthread_mutex_unlock(&mutex);  // -> ADDED TO SOLVE RACE CONDITION 1

    }

    return NULL;

}


int main(void) {

    // Signals handler ID (signalsThread()).
    pthread_t sig_thr_id; 

    // Creating a signals mask, that will block the SIGINT signal, and enabling
    // it for the current main thread.
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGINT);
    pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);

    // Creating the SIGINT signal thread handler.
    pthread_create(&sig_thr_id, NULL, signalsThread, NULL);

    while(1) {
        pthread_mutex_lock(&mutex);  // -> ADDED TO SOLVE RACE CONDITION 1

        printf("THREAD MAIN: %d.\n", secret);
        sleep(5); // -> USE WITH NO MUTEX TO OBTAIN THE RACE CONDITION 1 BEHAVIOUR.
        // The sleep is to "artificially force" a possible behaviour.

        //------------------------------------------------------------
        if (secret == 0) secret = 1;
        else secret = 0;
        //------------------------------------------------------------
        //sleep(5); // -> USE WITH NO MUTEX TO OBTAIN THE EXPECTED BEHAVIOUR.
        // The sleep is to "artificially force" a possible behaviour.

        pthread_mutex_unlock(&mutex); // -> ADDED TO SOLVE RACE CONDITION 1

        sleep(1);   // ALWAYS BE CAREFUL WHEN LOCKING/UNLOCKING A MUTEX IN A WHILE!
        // WITHOUT THIS LAST SLEEP, THE OTHER THREAD WILL NEVER ACQUIRE THE MUTEX (TESTED)!
        // BECAUSE THE MUTEX WILL BE RE-ACQUIRED FROM US.

    }

    return 0;

}

/*

///////////////////////////        WITHOUT MUTEX        ///////////////////////////         

NORMAL - EXPECTED:

THREAD MAIN     THREAD SIGNAL       SECRET VALUE MEMORY
      R                                       0 <- PRINTED
      F                                       1
                      R                       1 <- PRINTED
                      F                       0

OUTPUT
----------------
THREAD MAIN: 0.
CTRL + C THREAD: 1.
----------------




POSSIBLE RACE CONDITION 1:

THREAD MAIN     THREAD SIGNAL       SECRET VALUE MEMORY
      R                                       0 <- PRINTED
                      R                       0 <- PRINTED
      F                                       1
                      F                       0


OUTPUT
----------------
THREAD MAIN: 0.
CTRL+C THREAD: 0.
----------------

Let's try to use pthread_mutex_t to solve the issue (forcing the NORMAL - EXPECTED
behaviour while using the RACE CONDITION 1 setup)... 

///////////////////////////        WITH MUTEX        ///////////////////////////         

It's works, we obtain the expected behaviour, even if using the RACE CONDITION setup.

OUTPUT
----------------
THREAD MAIN: 0.
CTRL+C THREAD: 1.
----------------

*/

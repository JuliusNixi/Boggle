/*

    These are tests on signals and threads with sigwait() to better understand
    what happens when using signals and threads and do not cause troubles in the project,
    i will use only the SIGINT signal.

    I expect that the SIGINT signal will be always handled by the dedicated thread.

*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

// Signals to block.
sigset_t signal_mask;  

void* signalsThread(void* args) {

    // Sig number caught. Should be 2 == SIGINT.
    int signum; 

    while (1){

        // Waiting signals (SIGINT only).
        sigwait(&signal_mask, &signum);
    
        // To treat different signals.
        switch (signum){
            // Process SIGINT.
            case SIGINT:     
                printf("SIGNAL RECEIVED: %d. THREAD HANDLER ID: %lu.\n", signum, (unsigned long) pthread_self());
                break;
            default:     
                fprintf(stderr, "Unexpected signal... Code: %d.\n", signum);
                exit(EXIT_FAILURE);
                break;
        }

    }

    return NULL;

}

void* junkThread(void* args) {

    while(1) {
        sleep(2);
        printf("HELLO FROM THE JUNK THREAD, MY ID: %lu.\n", (unsigned long) pthread_self());
    }

    return NULL;

}


int main(void){

    // Signals handler thread ID (signalsThread()).
    pthread_t sig_thr_id; 

    // Creating a signals mask, that will block the SIGINT signal, and enabling
    // it for the current main thread.
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGINT);
    pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);

    /* Any newly created threads INHERIT the signals mask. */

    // Creating the thread that will handle the SIGINT blocked signal (and eventually others).
    // Is COUNTERINTUITIVE that the handler MUST HAVE the SIGNINT blocked, when
    // is the same one that should handle it.
    // But this is the implementation of the sigwait() and how it works.
    // From manual:
    /*
    [...] The signals specified by set should be blocked, but not ignored, at the
    time of the call to sigwait(). [...]
    */
    pthread_create(&sig_thr_id, NULL, signalsThread, NULL);

    // Junk test thread...
    pthread_t junk_thr_id;
    pthread_create (&junk_thr_id, NULL, junkThread, NULL);

    while(1) {
        sleep(1);
        printf("THREAD MAIN ID: %lu. PRESS CTRL + C TO SEND SIGINT.\n", (unsigned long int) pthread_self());
    }

    return 0;

}



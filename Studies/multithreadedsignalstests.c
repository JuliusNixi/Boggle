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

sigset_t signal_mask;  // Signals to block.

void* signalsThread(void* args) {

    int signum; // Sig number caught. Should be 2 == SIGINT.
    int retvalue;

    while (1){

        // Waiting signals (SIGINT only).
        retvalue = sigwait(&signal_mask, &signum);
        if (retvalue != 0) {
            printf("Error in sigwait().\n");
            exit(EXIT_FAILURE);
        }
    
        // To treat different signals.
        switch (signum){
            case SIGINT:     // Process SIGINT.
                printf("\n\nSIGNAL RECEIVED: %d. THREAD HANDLER ID: %lu.\n\n\n", signum, (unsigned long int) pthread_self());
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
        printf("HELLO FROM THE JUNK THREAD, MY ID: %lu.\n", (unsigned long int)pthread_self());
    }

    return NULL;

}


int main(void){

    pthread_t sig_thr_id; // Signals handler ID (signalsThread()).
    int retvalue;

    // Creating a mask, that will block the SIGINT signal, and enabling
    // it for the current main thread.
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGINT);
    retvalue = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
    if (retvalue != 0) {
        printf("Error in setting the pthread mask.\n");
        exit(EXIT_FAILURE);
    }

    /* Any newly created threads INHERIT the signals mask. */

    // Creating the thread that will handle the SIGINT blocked signal (and eventually others).
    // Is COUNTERINTUITIVE that the handler has the SIGNINT blocked, when
    // is the same one that should handle it.
    // But this is the implementation of the sigwait() and how it works.
    // From manual:
    /*
    [...] The signals specified by set should be blocked, but not ignored, at the
    time of the call to sigwait(). [...]
    */
    retvalue = pthread_create(&sig_thr_id, NULL, signalsThread, NULL);
    if (retvalue != 0) {
        printf("Error in creating the pthread signals handler.\n");
        exit(EXIT_FAILURE);  
    }

    // Junk test thread...
    pthread_t junk_thr_id;
    retvalue = pthread_create (&junk_thr_id, NULL, junkThread, NULL);
    if (retvalue != 0) {
        printf("Error in creating the junk pthread.\n");
        exit(EXIT_FAILURE);  
    }

    while(1) {
        sleep(1);
        printf("THREAD MAIN ID: %lu.\n", (unsigned long int) pthread_self());
    }

    return 0;

}



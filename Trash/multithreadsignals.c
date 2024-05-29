/*

    THESE ARE EXPERIMENT ON SIGNALS AND THREADS WITH SIGWAIT TO BETTER UNDERSTAND WHAT HAPPENS WHEN
    USING SIGNALS AND THREADS AND DO NOT CAUSE TROUBLE IN THE PROJECT,
    I WILL USE SIGINT.

    I expect that the SIGINT signal will be always handled by the dedicated thread.

*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

sigset_t   signal_mask;  /* signals to block         */

void* signal_thread(void* arg)
{
    int       sig_caught;    /* signal caught       */
    int       rc;            /* returned code       */

    while (1){
        rc = sigwait(&signal_mask, &sig_caught);
        if (rc != 0) {
            /* handle error */
            printf("Error in sigwait().\n");
        }
        printf("SIGNAL RECEIVED: %d. THREAD HANDLER: %lu.\n", sig_caught, (unsigned long int)pthread_self());
    }
    // Optional to treat different signals.
    switch (sig_caught)
    {
        case SIGINT:     /* process SIGINT  */

            break;
        default:         /* should normally not happen */
            fprintf(stderr, "\nUnexpected signal... Code: %d.\n", sig_caught);
            break;
    }
    return NULL;

}

void* junk_thread(void* arg) {

    while(1) {
        sleep(3);
        printf("HELLO FROM THE JUNK THREAD, MY ID: %lu.\n", (unsigned long int)pthread_self());
    }

    return NULL;

}


int main(int argc, char* argv[])
{
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

    // Junk test thread...
    pthread_t junk_thr_id;
    rc = pthread_create (&junk_thr_id, NULL, junk_thread, NULL);
    if (rc != 0) {
        // handle error 
        printf("Error in creating the junk pthread.\n");
        exit(EXIT_FAILURE);  
    }

    /* APPLICATION CODE */
    while(1) {
        sleep(3);
        printf("THREAD MAIN: %lu.\n", (unsigned long int) pthread_self());
    }

}



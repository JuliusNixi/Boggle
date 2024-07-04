#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

// Tests on blocking a signal on a thread during the execution of the program.

sigset_t signal_mask;
pthread_t t;

void* f(void* args) {

    printf("STARTED T ID: %lu.\n", (unsigned long) pthread_self());

    int i = 0;
    while (1) {
        printf("Thread noise...\n");
        if (i++ == 10) break;
        sleep(1);
    }

    printf("Blocking SIGUSR1...\n");
    
    // Changing and setting the signals mask for this thread.
    sigaddset(&signal_mask, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);

    while (1) {
        printf("Thread noise again... SIGUSR1 should now not disturbe anymore.\n");
        sleep(1);
    }

    return NULL;

}

void* signalsThread(void* a) {

    int sig;

    while(1) {

        sigwait(&signal_mask, &sig);

        switch (sig){
        case SIGALRM:
            printf("Sending SIGUSR1 to the other thread...\n");
            pthread_kill(t, SIGUSR1);
            break;  
        default:
            break;
        }

    }

    return NULL;
}

void sigusr1Handler(int signum) {

    printf("Handler SIGUSR1, thread ID: %lu.\n", (unsigned long) pthread_self());

}

int main(void) {

    // Creating the signals mask.
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGALRM);

    // Enabling the signals mask.
    pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);

    // SIGUSR1 handler setup.
    struct sigaction sigusr1;
    sigusr1.sa_flags = 0;
    sigusr1.sa_handler = sigusr1Handler;   
    sigaction(SIGUSR1, &sigusr1, NULL);

    // Starting signals handler thread.
    pthread_t sig_thr_id;
    pthread_create(&sig_thr_id, NULL, signalsThread, NULL);

    // Starting an other thread.
    pthread_create(&t, NULL, f, NULL);

    while (1){
        printf("Main, setting alarm...\n");
        alarm(1);
        sleep(5);
    } 

    return 0;

}



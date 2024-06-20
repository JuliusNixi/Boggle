
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

// Tests on blocking a signal on a thread after.

sigset_t signal_mask;
pthread_t t;


void* f(void* args) {

    printf("F ID: %lu.\n", (unsigned long)pthread_self());
    fflush(stdout);

    int i = 0;
    while (1) {
        printf("Thread noise...\n");
        fflush(stdout);
        if (i++ == 8) break;
        sleep(1);
    }

    printf("Blocking...\n");
    fflush(stdout);
    
    sigaddset(&signal_mask, SIGUSR1);
    int retvalue = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);

    while (1) {
        printf("Thread noise again...\n");
        fflush(stdout);
        sleep(1);
    }

    return NULL;


}

void* signalsThread(void* a) {

    int retvalue, sig;

    while(1) {

        retvalue = sigwait(&signal_mask, &sig);

        switch (sig)
        {
        case SIGALRM:
            printf("Sending kill...\n");
            pthread_kill(t, SIGUSR1);
            fflush(stdout);
            break;
        
        default:
            break;
        }

    }

    return NULL;
}

void sigusr1Handler(int signum) {

    fflush(stdout);
    printf("Handler ID: %lu.\n", (unsigned long)pthread_self());
    fflush(stdout);

}

int main(void) {

    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGINT);
    sigaddset(&signal_mask, SIGALRM);
    sigaddset(&signal_mask, SIGPIPE);

    // Enabling the mask.
    int retvalue = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);

    struct sigaction sigusr1;
    sigusr1.sa_flags = 0;
    sigusr1.sa_handler = sigusr1Handler;   
    retvalue = sigaction(SIGUSR1, &sigusr1, NULL);

    pthread_t sig_thr_id;
    retvalue = pthread_create(&sig_thr_id, NULL, signalsThread, NULL);

    retvalue = pthread_create(&t, NULL, f, NULL);

    while (1){
        printf("Main alarm...\n");
        alarm(2);
        sleep(4);
    } 

    return 0;

}



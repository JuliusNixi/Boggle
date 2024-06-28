// This file illustrates a client I/O management problem,
// to understand the problem better look at inputoutputasynctests.c.

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>

sigset_t signal_mask;

pthread_t maint;
pthread_t sig_thr_id;

#define TIMER_SECONDS 10

// This thread will handle with sigwait() only the SIGALRM signal.
void* signalsThread(void* args) {

    int sig;
    int retvalue;
    while (1) {
        retvalue = sigwait(&signal_mask, &sig);
        if (sig == SIGALRM) {

            // Simulating a server's response to print.
            printf("\n");
            printf("NOISE\n");
            fflush(stdout);
            // Interrupt the read() in the main thread.
            pthread_kill(maint, SIGUSR1);
            // Reset the timer.
            alarm(TIMER_SECONDS);

        }
    }
    return NULL;

}

// Nothing, just to interrupt the read() in the main thread.
void sigUSR1Handler(int signum) {
    return;
}

int main(void) {

    maint = pthread_self();

    // Blocking the SIGALRM to prepare it for the signalsThread().
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGALRM);
    int retvalue = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);

    // Creating the SIGALRM thread handler.
    retvalue = pthread_create(&sig_thr_id, NULL, signalsThread, NULL);

    // Setting the void SIGUSR1 that will be used to interrupt from the signalsThread(),
    // when the timer expires, the read() in the main thread.
    struct sigaction sigusr1; 
    sigusr1.sa_flags = 0;
    sigusr1.sa_handler = sigUSR1Handler;   
    retvalue = sigaction(SIGUSR1, &sigusr1, NULL);

    alarm(TIMER_SECONDS);

    // Input buffer.
    #define N 100
    char buffer[N];

    while (1) {

        // Printing the prompt.
        printf("-> ");
        fflush(stdout);

        // Reading the user's input.
        int retvalue = read(STDOUT_FILENO, buffer, N);

        if (retvalue > 0) {
            // Processing the user's input and clearing the buffer.
            printf("INSERTED: %s", buffer);
            memset(buffer, '\0', N);
            fflush(stdout);
        }else{

            // HERE THE STDIN BUFFER PROBLEM
            
            // fflush(stdin); // Not work.
            // scanf("%*[^\n]%*c"); // Works but remains waiting for more input.
            // int c; while ((c = getchar()) != '\n' && c != EOF); // Works but remains waiting for more input.
            // fpurge(stdin); // Not always available.

        }


    }

    return 0;

}


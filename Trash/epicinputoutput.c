#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
sigset_t signal_mask;
pthread_t maint;
pthread_t ccl;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
#define N 100
char buf[N];
int completed = 0;
int received = 0;
int ok = 0;
void* cleaner(void* a) {

    memset(buf, '\0', N);
    int c = 0;
    while (++c || getchar() != '\0') if (c >= N) completed = 1;
    return NULL;
}


void* f(void* args) {
    int sig;
    int retvalue = sigwait(&signal_mask, &sig);
    
    switch (sig){
        case SIGALRM:{ 
            
            ok = 2;

            while (1)
            {
                if (received) break;
                else pthread_kill(maint, SIGUSR1);
            }
            
            pthread_mutex_lock(&m);

            printf("\nBLA\nBLA\n");
            fflush(stdout);
            
            pthread_create(&ccl, NULL, cleaner, NULL);
            while (1)
            {  
                if (completed){
                    pthread_cancel(ccl);
                    break;
                } 
            }

            pthread_mutex_unlock(&m);

            ok = 1;
            
            break;
        }
    }
    return NULL;
}

void e(int sig){



}

void sn(){
    char* c = buf;
    while(c[0] != '\0'){
        if (c[0] == '\n') c[0] = '\0';
        c++;
    }
}

int main(void){


    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGALRM);
    int retvalue = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);

    maint = pthread_self();
    pthread_t t;
    pthread_create(&t, NULL, f, NULL);

    alarm(5);   

    struct sigaction sigusr1;
    sigusr1.sa_flags = 0;
    sigusr1.sa_handler = e;   
    retvalue = sigaction(SIGUSR1, &sigusr1, NULL);

    while (1) {
        pthread_mutex_lock(&m);
        printf("-> ");
        fflush(stdout);
        received = 0;
        int r = read(STDIN_FILENO, buf, N);
        received++;
        sn();
        if (r!=-1) printf("INSERTED: %s\n", buf);
        fflush(stdout);
        pthread_mutex_unlock(&m);
        while (1)
            if (ok == 0 || ok == 1){
                ok = 0;
                break;
            } 
    }


    return 0;

}





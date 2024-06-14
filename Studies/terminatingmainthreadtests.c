#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

// Tests on terminating before the main thread compared to others threads.

void* f(void* args){

    while (1){
        sleep(1);
        printf("Hello from the other thread!\n");
    }
    
    return NULL;

}

int main(void) {


    pthread_t t;
    pthread_create(&t, NULL, f, NULL);

    int counter = 0;
    while (1){
        sleep(1);
        counter++;
        printf("MAIN COUNTER: %d.\n", counter);
        if (counter == 5) break;
    } 

    printf("MAIN EXITING...\n");
    fflush(stdout);

    return 0;


}


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

// We can test two different exit methods for the main thread.
// Set this to 0 or 1 to see the differences.
char exitmethod1or2 = 1; 

int main(void) {

    // Creating the other thread.
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

    if (exitmethod1or2 == 0) return 0;
    else pthread_exit(NULL);

}


#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

void* f(void* args){

    while (1){
        sleep(1);
        printf("Hello!\n");
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
        if (counter == 4) return 0;
    } 


    return 0;


}


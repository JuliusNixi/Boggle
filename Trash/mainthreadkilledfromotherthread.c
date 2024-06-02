
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

pthread_t maint;
void* f(void* args) {

    sleep(5);
    printf("Killing main...\n");
    pthread_cancel(maint);
    return NULL;


}

void atExit(void) {

    printf("Exiting...\n");

}

int main(void) {

    atexit(atExit);
    maint = pthread_self();
    pthread_t t;
    pthread_create(&t, NULL, f, NULL);


    while (1)
    {
        printf("Hello!\n");
        sleep(3);
    }
    


    return 0;

}




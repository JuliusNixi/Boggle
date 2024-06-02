#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>


void c(void* a) {

    printf("Hello!\n");
    fflush(stdout);

}


void thread_local_destructor(void* arg) {

  c(NULL);
  
}

pthread_key_t key;
pthread_once_t key_once = PTHREAD_ONCE_INIT;

void make_key(){

    pthread_key_create(&key, thread_local_destructor);

}

void func() {

    void *ptr;

    pthread_once(&key_once, make_key);
    if ((ptr = pthread_getspecific(key)) == NULL) {
        ptr = malloc(sizeof(char));
        // ...
        pthread_setspecific(key, ptr);
    }
    
}


void* f(void* args){

    int i;
    func();
    while(1) {
        printf("i: %d.\n", i++);
        fflush(stdout);
        sleep(1);
        if (i == 4) break;
    }
    return NULL;
}

int main(void) {
    
    pthread_t t;
    pthread_create(&t, NULL, f, NULL);
    
    while(1) {
        printf("Main...\n");
        fflush(stdout);
        sleep(1);
    }

    return 0;

}



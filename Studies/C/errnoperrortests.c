#include <stdio.h>
#include <errno.h>
#include <pthread.h>

// Tests on errno and perror.

void* f(void* args) {

    errno = 2;
    perror("ERRNO IN THE THREAD");

    return NULL;

}

int main(void) {

    // Default errno (0).
    printf("DEFAULT ERRNO: %d.\n", errno); 
    perror("DEFAULT PERROR");

    // Manual setting errno to see if it could be done.
    errno = 1;
    perror("ERRNO SETTED TO 1");

    pthread_t t;
    pthread_create(&t, NULL, f, NULL);
    pthread_join(t, NULL);

    perror("ERRNO AGAIN FROM MAIN (SHOULD REMAIN 1)");

    // So there is a errno different in each thread.

    return 0;

}



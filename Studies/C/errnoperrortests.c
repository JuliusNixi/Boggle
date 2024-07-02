#include <stdio.h>
#include <errno.h>

// Tests on errno and perror.

int main(void) {

    // Default errno (0).
    printf("DEFAULT ERRNO: %d.\n", errno); 
    perror("DEFAULT PERROR");

    // Manual setting errno to see if it could be done.
    errno = 1;
    perror("ERRNO SETTED TO 1");

    return 0;

}



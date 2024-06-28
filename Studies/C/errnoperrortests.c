#include <stdio.h>
#include <errno.h>

// Tests on errno.

int main(void) {

    // Default errno (0).
    printf("ERRNO: %d.\n", errno); 
    perror("error");

    // Manual setting errno.
    errno = 1;
    perror("error");

    return 0;

}



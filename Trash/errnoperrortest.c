#include <stdio.h>
#include <errno.h>


int main(void) {

    printf("ERRNO: %d.\n", errno);
    perror("hello");
    errno = 1;
    perror("hello");

    return 0;

}



#include <stdio.h>

// Simply testing a write on the stderr of an error with fprintf().

int main(void) {

    fprintf(stderr, "Error.\n");

    return 0;

}

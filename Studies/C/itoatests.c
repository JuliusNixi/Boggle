#include <stdio.h>
#include <math.h>

// This tests file contains some tests on "getting the number of digits of a number".
// The idea was found on StackOverflow, but unfortunately i lost the link.

int main(void) {

    unsigned long n = 0LU;
    unsigned long ndigits = n <= 9 ? 1LU : floor (log10 ( (n))) + 1LU;
    printf("N: %lu. Digits: %lu.\n", n, ndigits);

    n = 10LU;
    ndigits = n <= 9 ? 1LU : floor (log10 ( (n))) + 1LU;
    printf("N: %lu. Digits: %lu.\n", n, ndigits);
    
    n = 100LU;
    ndigits = n <= 9 ? 1LU : floor (log10 ( (n))) + 1LU;
    printf("N: %lu. Digits: %lu.\n", n, ndigits);

    return 0;

}


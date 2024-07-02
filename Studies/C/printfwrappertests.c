#include <stdarg.h>
#include <stdio.h>

// These tests are used to understand how stdarg works (functions with variable number of parameters).
// The ultimate goal is to create a wrapper of the printf function.
// At the end, this idea was never applied to the project because it was not useful for
// what I wanted to achieve.

void printff_2(int y, const char* format, va_list args) {

    printf("Various received args (va_list):\n");
    // The vprintf() is needed to works with stdarg.
    vprintf(format, args);
    printf("Other arg (no va_list): %d.\n", y);

}

// Testing a printf wrapper.
void printff(int x, char* format, ...) {

    va_list args;
    va_start(args, format);

    // Incrementing x.
    x++;
    // Forwarding to the other funciton.
    printff_2(x, format, args);

    va_end(args);

}


int main(void) {


    printff(42, "Hello %d %d.\n", 2, -1);
    return 0;


}



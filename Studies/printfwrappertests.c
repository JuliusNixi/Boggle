#include <stdarg.h>
#include <stdio.h>

// These tests are used to understand how stdarg works (functions with variable number of parameters).
// The ultimate goal is to create a wrapper of the printf function.

void printff_2(int y, const char* format, va_list args) {

    printf("Various received args (va_list):\n");
    vfprintf(stdout, format, args);
    fprintf(stdout, "Other arg (no va_list): %d.\n", y);

}

// Testing a printf wrapper.
void printff(int x, char* format, ...) {

    va_list args;
    va_start(args, format);

    x++;
    printff_2(x, format, args);

    va_end(args);

}


int main(void) {


    printff(42, "Hello %d %d.\n", 84, -1);
    return 0;


}



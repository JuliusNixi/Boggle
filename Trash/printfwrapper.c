#include <stdarg.h>
#include <stdio.h>

void printff_2(int y, const char* format, va_list args) {

    vfprintf(stdout, format, args);
    fprintf(stdout, "Other args: %d.\n", y);

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


    printff(42, "Hello %d.\n", 84);
    return 0;


}



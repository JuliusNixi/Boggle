#include <stdio.h>
#include <string.h>

// Some tests on the strtok().
// Used to tokenise strings.

// WARNING: It's destroy the string on which it's working.

int main(void){

    char str[] = ",hello,mum,";

    // First call MUST BE with the str pointer, then with NULL.
    char* s = strtok(str, ",");
    printf("X 0: %s\n", s);

    s = strtok(NULL, ",");
    printf("X 1: %s\n", s);

    s = strtok(NULL, ",");
    printf("X 2: %s\n", s);

    s = strtok(NULL, ",");
    printf("X 3: %s\n", s);

    s = strtok(NULL, ",");
    printf("X 4: %s\n", s);

    printf("INITIAL STR: %s\n", str);

    // The function substitute the next founded token with '\0'.

    return 0;

}



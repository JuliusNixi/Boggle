#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// This file contains a simply test to chose a random char from a string.

int main(void) {

    // Setting random seed.
    srand(42);

    char teststring[] = "ABCDE";

    while(1) {
        int randint = rand() % strlen(teststring);
        printf("Choosen: %c.\n", teststring[randint]);
        sleep(1);
    }


    return 0;

}


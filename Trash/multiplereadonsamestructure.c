#include <unistd.h>
#include <stdio.h>

/*

This file is a test to see if it is possible to write data structures represented by multiple bytes,
with multiple different read() calls by properly updating the generic void* pointer used. 

After testing it is confirmed, I can use multiple reads to write data simply by incrementing 
the void pointer accordingly.


*/

int main(void) {

    printf("SIZE OF UNSIGNED INT: %lu.\n", sizeof(unsigned int));
    // Expected 4 byte.

    // Unsigned rapresentation. 1 byte = 2^8 different numbers rapresented.
    // So it can handles numbers from 0 to 255.
    unsigned int x = 0U;
    void* p = &x;
    // The actual value of x is 0.
    // Inserting from STDIN a single char 'a' and writing in x it with the void* p pointer.
    read(STDIN_FILENO, p, 1);
    // 'a' is 97 as code ASCII.
    // So 97 < 255. Only the first byte is written.
    // I expect to see exactly 97 if now i print x.
    printf("%u.\n", (x));

    // To remove '\n'.
    read(STDIN_FILENO, NULL, 1);

    // Unsigned rapresentation. 1 byte = 2^8 different numbers rapresented.
    // So it can handles from 0 to 255. Now let's set 256.
    x = 256U;
    // The actual value of x is 256, so the first AND second byte are used.
    // Inserting from STDIN a single char 'a' and writing in x it with the void* p pointer.
    read(STDIN_FILENO, p, 1);
    // 'a' is 97 as code ASCII.
    // So 97 < 255. Only the first byte is written BUT the second byte contains a 1 (255 first
    // + 1 second).
    // So by subtracting 97, the first byte will be all 0, and will remain a 1 in the second byte.
    // So i will expect again a 256.
    x = x - 97U;
    printf("%u.\n", (x));

    // p has been increased by 1 byte.
    p++;

    // To remove '\n'.
    read(STDIN_FILENO, NULL, 1);

    // Resetting x.
    x = 0U;
    // Insert '\n', simply press enter, its ASCII value is 10.
    read(STDIN_FILENO, p, 1);
    // The first byte is now all 0. The second should contain now 10 (\n inserted).
    // So the entire number will be 0000101000000000 -> 2560 converted in decimal.
    printf("%u.\n", (x));

    return 0;

}


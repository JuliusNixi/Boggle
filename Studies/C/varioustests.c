// Various tests.

#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(void) {

    // Insert a string, for example "hello" and press enter.
    // The 'h' letter will be read and inserted in c.
    printf("INSERT A WORD\n");
    fflush(stdout);
    char c[1]; 
    read(STDIN_FILENO, c, 1);
    printf("C: %c\n", *c);
    // Trying to flush STDIN.
    // We want to discard "ello\n" present in the STDIN buffer.
    fflush(stdin);
    // We expect a new prompt and not to see "ello\n" if it works.
    #define N 100
    char res[N];
    int retvalue = read(STDIN_FILENO, res, N);
    if (retvalue) printf("NO WORKS, IN STDIN STILL -> %s",res);
    // This not work... fflush() does not work with STDIN... :(

    // Resetting.
    // STDIN now clear.
    memset(res, 0, N); // Resetting buffer.

    // Let's do another try...
    // Can i write manually in STDIN?
    char test[] = "hello\r\n\0";
    write(STDIN_FILENO, test, strlen(test) * sizeof(char));
    read(STDIN_FILENO, res, N); // This not read hello, waits for input...
    printf("RES: %s", res);
    // Not works as wanted, because the write is displayed, but cannot be readed after
    // and it also print a random char.

    // Resetting.
    printf("INSERT SOMETHING TO CLEAR STDIN\n");
    read(STDIN_FILENO, res, N); // Clearing the STDIN.
    memset(res, 0, N); // Resetting buffer.
    printf("ALL RESETTED\n");

    // Testing if read() read also '\0'.
    printf("INSERT A CHAR\n");
    char c3[3]; // A char + 1 for '\n' + 1 for '\0'.
    retvalue = read(STDIN_FILENO, c3, 3);
    printf("R: %d. S: %s", retvalue, c3);
    // A \n     r == 2 (only 2 bytes read))
    // The string terminator must be inserted manually.
    c3[2] = '\0';

    // Testing if read() can have a NULL buffer.
    printf("YOU MUST INSERT 2 CHARS\n");
    retvalue = read(STDIN_FILENO, NULL, 3); // 2 for XY chars + 1 for '\n'.
    printf("WITH NULL BUFFER IN READ, RESULT CODE: %d.\n", retvalue);
    perror("Read error");
    // r == -1, error, bad address, cannot use NULL.
    // X is read from STDIN, but not Y, so now the STDIN content is "Y\n"
    retvalue = read(STDIN_FILENO, c3, 2);
    printf("R: %d. S: %s", retvalue, c3);

    // Testing strlen.
    char lentest[] = "hello";
    char lentest2[] = "hello\0";
    char lentest3[] = "h\0ello\0";
    printf("LEN: %lu\n", strlen(lentest)); // 5
    printf("LEN 2: %lu\n", strlen(lentest2)); // 5
    printf("LEN 3: %lu\n", strlen(lentest3)); // 1
    // So strlen() stops to the first '\0', not including it in the returned number.

    return 0;
    
}



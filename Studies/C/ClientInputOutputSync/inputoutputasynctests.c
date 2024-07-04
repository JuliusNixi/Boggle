// This file contains all the tests needed to develop the async project's client.
// Async, in this case, means that the client waits for the user's input in a ThreadA.
// A ThreadB stays listening to capture responses from the server
// and places them in a messages list. 
// Periodically, the ThreadA interrupts the read() of the user input and checks if there 
// are any server responses in the list to be printed.
// In this latter case, it prints the server's responses and then come back
// to waits for user's input after clearing the STDIN buffer.
// If there's no responses received from the server, the read() user's input come back to where
// it was left and the user doesn't notice anything, the STDIN buffer content persists.

// CLARIFICATIONS: This client implementation is not 100% asynchronous in the true meaning
// of the word, the server's responses are received from the server, and inserted into the list
// asynchronously by the ThreadB, but aren't printed INSTANTANEOUSLY ASYNC,
// instead, at regular intervals, the ThreadA stops waiting for user's input and checks
// for server's responses in the list and prints them, but since this time interval
// can be chosen very low, the user will perceive almost a pure asynchronous behavior.

/*

The key concept on which the client development was structured is the cleanup of the printing
of server's responses, and the subsequent printing (and input handling) of the prompt.

It may seem superficial, but without proper attention, the GUI can be flawed sometimes.
It may seem like a little detail, but it has been very difficult and tedious to cope with
this need.

In particular, the problem was that when a response from the server is received and printed,
the user might be in the middle of a typing and have already entered characters into the STDIN
buffer.

To see the problem in action take a look at "./brokeninputoutput.c".

I identified two different ways to deal with this problem:

1) Prevent the problem (of the "dirty" STDIN buffer).
Develop the client as a single thread, which waits for the user to complete the input
(pressing ENTER which via '\n' interrupts the read()) and then executes the command and
finally checks if there are any queued server's responses to be printed and afterwards starts
the loop again.
Or, alternatively, develop the client as multithreaded and synchronize the thread that handles
the input with the one that prints the server's responses. 
Both solutions, however, have the same fallacy, the read() must be compulsorily blocking,
to be sure that it reads the entire STDIN buffer.

2) We cure the problem, let the STDIN buffer allowed to be "dirty" and flush it when necessary.
It sounds simple, but it is not so.... 
I have tried a lot of methods and read these discussions:
https://stackoverflow.com/questions/2979209/using-fflushstdin
https://stackoverflow.com/questions/2187474/i-am-not-able-to-flush-stdin-how-can-i-flush-stdin-in-c
https://stackoverflow.com/questions/36715002/how-to-clear-stdin-before-getting-new-input
Never a final solution...
I ended up going crazy with a library called ncurses:
https://en.wikipedia.org/wiki/Curses_(programming_library)
That allows you to manage the terminal in detail, but it was like using bazooka to kill a fly.
Solving the problem but generated many others unnecessary complications.

I finally found and adapted a method used in this below code.

To summarize, I interrupt the read() very often.
If there are server's responses I print them, clean up the STDIN with a NON-BLOCKING getchar()
and return to the clean prompt, so if the user has typed something incomplete can retype
it or changes it (following the response received seen). Instead, if there is no server response,
the read resumes with the user's entered STDIN and nothing happens.

See "./termiostests.c" to better understand how the termios lib is used.

*/

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

// This can be generalized to inputs of arbitrary length.
// But for these tests we don't care.
#define N 100
char input[N + 1]; // +1 for the '\0'.

#define PROMPT_STR "--> "

#define CHECK_RESPONSES_MICROSECONDS 100
struct termios oldChars;
struct termios newChars;
int oldfl;
char printprompt;

// This function is used to transform the getchar() function from blocking to unblocking.
void setUnblockingGetChar(void){

    // Saving current fcntl settings.
    oldfl = fcntl(0, F_GETFL); 
    if (oldfl == -1) {
            // Error
    }
    // Setting getchar() to unblocking.
    int retvalue = fcntl(0, F_SETFL, O_NONBLOCK); 
    if (retvalue == -1) {
        // Error
    }
    // Saving current terminal settings.
    retvalue = tcgetattr(0, &oldChars); 
    if (retvalue == -1) {
        // Error
    }
    newChars = oldChars;
    // Disabling buffering.
    newChars.c_lflag &= ~ICANON; 
    // newChars.c_lflag &= echo ? ECHO : ~ECHO; // Modifying echo mode.
    // Setting new terminal settings.
    retvalue = tcsetattr(0, TCSANOW, &newChars); 
    if (retvalue == -1) {
        // Error
    }

}

// This function is used to restore the getchar() function from unblocking to blocking.
void setBlockingGetChar(void) {

    // Setting old fcntl settings.
    int retvalue = fcntl(0, F_SETFL, oldfl & ~O_NONBLOCK);
    if (retvalue == -1) {
        // Error
    }
    // Setting old terminal settings.
    tcsetattr(0, TCSANOW, &oldChars);  
    if (retvalue == -1) {
        // Error
    }

}


// This function clears the user's input.
// It clears the input buffer.
// It clears the user's STDIN buffer, for that an unblocking getchar() was needed.
// It returns 1 if at least one char has been removed from STDIN, 0 otherwise.
int clearInput(void) {

    // Resetting input buffer.
    memset(input, '\0', N);

    // Clearing STDIN buffer.
    int cleared = 0;
    setUnblockingGetChar();
    while (1) {
        char c = getchar();
        // No more chars in the STDIN, break.
        if (c == -1) break;
        else cleared = 1;
    }
    setBlockingGetChar();

    return cleared;

}

int main(void) {

    // Initializing seed.
    srand(42);

    printprompt = 1;

    while(1) {

        // Periodically stop the read() and check for server's responses to print.
        // This is NOT about changing the mode of the getchar() function.
        // This MUST be done every while loop.
        fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
    
        // Printing prompt.
        if (printprompt == 1)
            printf(PROMPT_STR);
        else printprompt = 0;

        fflush(stdout);

        // Waiting for the user's input.
        // After this time the read() is automatically interrupted to check if there
        // are server's responses to print.
        // if there are not, it is taken back and the user does not notice it.
        usleep(CHECK_RESPONSES_MICROSECONDS);

        // Reading input.
        int retvalue;
        retvalue = read(STDIN_FILENO, input, N);

        if (retvalue > 0) {

                // Terminating the line.
                input[retvalue] = '\0';

                // Something has been read (r > 0).

                // MAY or MAY NOT have read ALL the data that the user would want to.
                // The user may have entered LESS data than N but with '\n', so it's completed,
                // OR the input is NOT completed ('\n' missing).
                
                // Checking if the input contains a '\n', to detect if it is completed or not,
                // to know if it's necessary or not printing the newline and prompt and 
                // to process the input.
                char* c = input;
                unsigned int counter = 0U;
                while(c[0] != '\0'){
                    if (c[0] == '\n'){
                        c[0] = '\0';
                        counter++;
                    } 
                    c++;
                }
                if (counter == 0 ? 0 : 1) {
                    // The input is completed, we can process it.
                    printf("INSERTED: %s.\n", input);
                    // Processing...
                    printprompt = 1;
                }else {
                    // The input is not completed, clearing the input. Maybe there is 
                    // a situation like this:
                    // -> USERINPUT...

                    // To know what to do (printing and clearing input), we need to know 
                    // whether there are server's responses to print or not.
                    // So we delegate this task to after, setting this flag.
                    printprompt = 2;

                }
        }else{
            // retvalue <= 0.
            // errno == 35 on macOS.
            // errno == 11 on Linux.
            if (errno == 35 || errno == 11){
                // Normal interrupt to display server's responses received.
                // Two cases:
                // 1. ->
                // 2. -> USERINPUT...

                // To know what to do (printing and clearing input), we need to know 
                // whether there are server's responses to print or not.
                // So we delegate this task to after, setting this flag.
                printprompt = 3;

            }else {
                // Unexpected strange error.
                printf("Unexpected error.\n");
                _exit(1);
            }
        }

    // Checking for server's responses.
    // Generate a random number and decide whether to simulate the printing of some server's
    // responses or not.
    char somethingtoprint = (rand() % 1000000) >= 999990;

    if (somethingtoprint) {

        if (printprompt == 2) {
            // The input is not completed, clearing the input. Maybe there is 
            // a situation like this:
            // -> USERINPUT...
            clearInput();
            printf("\n");
        }

        if (printprompt == 3) {
            // Two cases:
            // 1. ->
            // 2. -> USERINPUT...
            int i = clearInput();
            if (i == 0) {
                // First case.
                printf("\n");
            }else {
                // Second case.
                printf("\n");
            }
            // No difference.
            // But for readability better to distinguish them.
        }

        if (printprompt == 1) {
            ;
            // Nothing to do.
            // But for readability better to distinguish.
        }

        // Useless placeholder text that rapresent the server's response to print.
        printf("NOISE\nNOISE\n");

        printprompt = 1;

    }else{

        // Nothing from server to print.

        // No printing prompt.
        // Resetting flag.
        if (printprompt == 2)
            printprompt = 0;
        if (printprompt == 3)
            printprompt = 0;

        // The prompt must be printed.
        if (printprompt == 1)
            ;

        // The prompt must NOT be printed.
        if (printprompt == 0)
            ;
        // Nothing to do.
        // For readability better to distinguish them.

    }

        fflush(stdout);

    }

    return 0;

}


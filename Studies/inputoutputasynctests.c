// This file contains all the tests needed to develop the async client.
// Async, in this case, means that the client waits for the user's input, but
// periodically, checks if there are some received server's responses.
// In this latter case, it prints the server's responses and then come back
// to waits for user's input.

// CLARIFICATIONS: It's not asynchronous in the true meaning of the word,
// when a response is received from the server, it is NOT printed INSTANTANEOUSLY,
// instead, at regular intervals, it is checked for server's responses and printed them,
// but since this time interval can be chosen very low, the user will perceive almost
// pure asynchronous behavior.

/*
The key concept on which the client development was structured is the cleanup of the printing
of server's responses, and the subsequent printing (and input handling) of the prompt.

It may seem superficial, but without proper attention, the GUI can be flawed sometimes.
It may seem like a little detail, but it has been very difficult and tedious to cope with
this need.

In particular, the problem was that when a response from the server is received and printed,
the user might be in the middle of a typing and have already entered characters into the STDIN
buffer.

To see the problem in action take a look at brokeninputoutput.c.

I identified two different ways to deal with the problem:

1) Prevent the problem (of the "dirty" STDIN buffer).
Develop the client as a single thread, which waits for the user to complete the input
(pressing ENTER which via '\n' interrupts the read()) and then execute the command and
finally check if there are any queued server's responses to be printed and afterwards start
the loop again.
Or, alternatively, develop the client as multithreaded and synchronize the thread that handles
the input with the one that prints the server's responses. 
Both solutions, however, have the same fallacy, the read() must be compulsorily blocking,
to be sure that it reads the entire STDIN buffer.

2) We cure the problem, let the STDIN buffer be allowed to "dirty" and flush it when necessary.
It sounds simple, but it is not so.... 
I have tried a lot of methods and read these discussions:
https://stackoverflow.com/questions/2979209/using-fflushstdin
https://stackoverflow.com/questions/2187474/i-am-not-able-to-flush-stdin-how-can-i-flush-stdin-in-c
https://stackoverflow.com/questions/36715002/how-to-clear-stdin-before-getting-new-input
Never a final solution...
I ended up going crazy with a library called "curses.h":
https://en.wikipedia.org/wiki/Curses_(programming_library)
That allows you to manage the terminal in detail, but it was like using bazooka to kill a fly.
Solbing the problem but generated many others.

I finally found and adapted a method used in this below code.

To summarize, I interrupt the read() very often.
If there are server's responses I print them, clean up the STDIN with a NON-BLOCKING getchar()
and return to the clean prompt, so if the user has typed something incomplete they can retype
it or change it (following the response received). Instead, if there is no server response,
the read resumes with the user's entered STDIN and nothing happens.

See termiostest.h to better understand.
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
char input[N];

#define PROMPT "-> "

#define TIME_CHECK_RESPONSES_MICROSECONDS 100

struct termios oldChars;
struct termios newChars;

int oldfl;

int printprompt = 1;

void setUnblockingGetChar(){
  oldfl = fcntl(0, F_GETFL); // Saving current fcntl settings.
  fcntl(0, F_SETFL, O_NONBLOCK);
  tcgetattr(0, &oldChars); // Saving current terminal settings.
  newChars = oldChars;
  newChars.c_lflag &= ~ICANON; // Disabling buffering.
  // newChars.c_lflag &= echo ? ECHO : ~ECHO; // Modifying echo mode.
  tcsetattr(0, TCSANOW, &newChars); // Setting new terminal settings.
}

void setBlockingGetChar(){
    fcntl(0, F_SETFL, oldfl & ~O_NONBLOCK); // Setting old fcntl settings.
    tcsetattr(0, TCSANOW, &oldChars);  // Setting old terminal settings.
}

// This function substitute all the '\n' with '\0' in the input. 
// It returns 1 if at least a '\n' is found in the input, 0 otherwise.
int sanitizeCheckInput(){

    char* c = input;
    int counter = 0;
    while(c[0] != '\0'){
        if (c[0] == '\n'){
            c[0] = '\0';
            counter++;
        } 
        c++;
    }
    return counter == 0 ? 0 : 1;

}

// This function clears the user's input.
// It clears the input buffer.
// It also clears the user's STDIN buffer, for that an unblocking getchar() was needed.
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

    // This extra line is needed because the fcntl is changed.
    // It has been restored to the original, but this change is missing (also in the original).
    // It is needed to periodically stop the read() and check for server's responses to print.
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

    return cleared;

}

// Simulate received responses from server.
void printResponses(void) {
    
    // Generate a random number and decide whether to simulate the printing of some server's
    // responses or not.
    int somethingtoprint = rand() % 1000000 >= 999990;

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
        
    return;

}

int main(void) {

    // Initializing seed.
    srand(42);

    // Initializing is needed (to set the read() timeout).
    clearInput();

    while(1) {

        fflush(stdout);

        // Printing prompt.
        if (printprompt == 1)
            printf(PROMPT);
        else printprompt = 0;

        // Waiting for the user's input.
        // After this time the read() is automatically interrupted to check if there
        // are server's responses to print.
        // However, it is taken back and the user does not notice it.
        usleep(TIME_CHECK_RESPONSES_MICROSECONDS);

        // Reading input.
        int retvalue;
        retvalue = read(STDIN_FILENO, input, N);

        if (retvalue != -1) {
                // Something has been read (r > 0) or r == 0.

                // MAY or MAY NOT have read ALL the data that the user would want to.
                // The user may have entered LESS data than N but with '\n', so it's completed,
                // OR a signal is arrived and so the input is NOT completed ('\n' missing).
                
                // If a read is interrupted by a signal...  
                // https://linux.die.net/man/3/read
                // https://stackoverflow.com/questions/66987029/can-a-read-or-write-system-call-continue-where-it-left-off-after-being-inter

                // Checking if the input contains a '\n', to detect if it is completed or not,
                // to know if it's necessary or not printing the newline and prompt and 
                // to process the input.
                if (sanitizeCheckInput()) {
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
                    // So we defer this task to the function printResponses(), setting this flag.
                    printprompt = 2;

                }
        }else{
            // -1.
            if (errno == 35){
                // Interrupt to display server's responses received.

                // Two cases:
                // 1. ->
                // 2. -> USERINPUT...

                    // To know what to do (printing and clearing input), we need to know 
                    // whether there are server's responses to print or not.
                    // So we defer this task to the function printResponses(), setting this flag.
                printprompt = 3;

            }else {
                // Unexpected strange error.
                printf("Unexpected error.\n");
                fflush(stdout);
                _exit(1);
            }
        }

        // Checking for server's responses.
        printResponses();

        fflush(stdout);

    }

    return 0;

}


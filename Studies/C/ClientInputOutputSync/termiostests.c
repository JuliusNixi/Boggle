// What is this?
// Read the "./inputoutputasynctests.c" file.

// Thanks to...
// https://gamedev.stackexchange.com/questions/146256/how-do-i-get-getchar-to-not-block-the-input
// https://stackoverflow.com/questions/388434/linux-fcntl-unsetting-flag
// Others note:
// https://www.man7.org/linux/man-pages/man2/fcntl.2.html

#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

struct termios oldChars;
struct termios newChars;

int oldfl;

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

int main(void){

    printf("CHANGING getchar() MODE TO UNBLOCKING...\n");
    setUnblockingGetChar();

    // Now getchar() should be unblocking.
    printf("Type a char, it should be readed instantly...\nType 'A' to exit.\n");
    while (1){
        char c = getchar();
        if (c != -1) printf("C: %c %d.\n", c, (int) c);
        sleep(1);
        if (c == 'A') break;
    }

    // Resetting terminal settings.
    printf("CHANGING getchar() MODE TO BLOCKING...\n");
    setBlockingGetChar();

    // Now getchar() should be again blocking.
    printf("Now getchar() should be blocking again...\nType a word like 'hello' and press ENTER.\n");
    while(1) {
        char c = getchar();
        if (c == 'A') break;
        if (c == '\n') continue;
        printf("C: %c %d.\n", c, (int) c);
    }

    return 0;

}
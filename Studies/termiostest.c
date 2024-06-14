// Why all this complication?
// Read inputoutputasynctests.c.
// TL;DR: Because for the client development i need an unblocking getchar() to clear the
// STDIN buffer when an interrupt happens (e.g. print server's responses)
// during the user's typing.

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

int main(void){

    printf("CHANGING MODE TO UNBLOCKING...\n");
    setUnblockingGetChar();

    // Now getchar() should be unblocking.
    printf("Type a char, it should be readed instantly...\nType 'A' to exit.\n");
    while (1){
        char c = getchar();
        if (c != -1) printf("C: %c %d.\n", c, (int) c);
        sleep(3);
        if (c == 'A') break;
    }

    // Resetting terminal settings.
    printf("CHANGING MODE TO BLOCKING...\n");
    setBlockingGetChar();

    // Now getchar() should be again blocking.
    printf("Now getchar() should be blocking again...\n");
    while(1) {
        char c = getchar();
        if (c == 'A') break;
        if (c == '\n') continue;
        printf("C: %c %d.\n", c, (int) c);
    }

    return 0;

}
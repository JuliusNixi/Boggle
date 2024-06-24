// Shared client cross files vars and libs.
#include "client.h"

// Current file vars and libs.
#include <fcntl.h>
#include <termios.h>

#define PROMPT_STR "[PROMPT BOGGLE]--> " // Prompt string.

#define HELP_MSG "Avaible commands:\nhelp -> Show this page.\nregister_user user_name -> To register in the game.\nmatrix -> Get the current game matrix.\np word -> Submit a word.\nend -> Exit from the game.\n" // Help message.

char* inputfinal;  // This heap allocated string will contain the full user input.
char input[BUFFER_SIZE + 1]; // An input temporary buffer, we will handle arbitrary input length by using more BUFFER_SIZE heap allocated strings in a list. +1 for the '\0'.

#define CHECK_RESPONSES_MICROSECONDS 100 // Time in microseconds. Each time, the responses received from the server (if present), will be printed.
struct termios oldChars; // To save current terminal stteings.
struct termios newChars; // To set new terminal stteings.
int oldfl;
char printprompt = 1; // This var is used to determine what to print and what not to print.

struct StringNode { // Support struct used to grab the user input of arbitrary length in the client using a heap allocated linked list of char* (these also heap allocated).
    char* s;   // Pointer to an heap allocated string.
    struct StringNode* n; // Pointer to the next list element.
};
struct StringNode* heads = NULL;

struct MessageNode { // This will be a messages list. Will be filled asynchronously when a server's response is received by the responsesHandler() thread. The presence of the response in the list indicates that it has yet to be printed (shown to the user).
    struct Message* m; // Pointer to the Message struct.
    struct MessageNode* next; // Pointer to the next element of the list.
};
struct MessageNode* head = NULL; // Head of the messages list.
pthread_mutex_t listmutex = PTHREAD_MUTEX_INITIALIZER; // This mutex is used by the threads to synchronize on messages list operations.

// This function destroy the strings list.
void destroyStringList(void) {

    struct StringNode* current = heads;
    while (1) {
        if (current == NULL) break;
        memset(current->s, '\0', BUFFER_SIZE + 1);
        free(current->s);
        struct StringNode* tmp = current->n;
        free(current);
        current = tmp;
    }
    heads = NULL;

}

// This function is used to transform the getchar() function from blocking to unblocking.
void setUnblockingGetChar(void){

  oldfl = fcntl(0, F_GETFL); // Saving current fcntl settings.
  if (oldfl == -1) {
        // Error
        handleError(1, 1, 0, 0, "Error in fcntl() in setUnblockingGetChar().\n");
   }
  int retvalue = fcntl(0, F_SETFL, O_NONBLOCK);
  if (retvalue == -1) {
    // Error
    handleError(1, 1, 0, 0, "Error in fcntl() in setBlockingGetChar().\n");
  }
  tcgetattr(0, &oldChars); // Saving current terminal settings.
  newChars = oldChars;
  newChars.c_lflag &= ~ICANON; // Disabling buffering.
  // newChars.c_lflag &= echo ? ECHO : ~ECHO; // Modifying echo mode.
  tcsetattr(0, TCSANOW, &newChars); // Setting new terminal settings.

}

// This function is used to restore the getchar() function from unblocking to blocking.
void setBlockingGetChar(void) {

    int retvalue = fcntl(0, F_SETFL, oldfl & ~O_NONBLOCK); // Setting old fcntl settings.
    if (retvalue == -1) {
        // Error
        handleError(1, 1, 0, 0, "Error in fcntl() in setBlockingGetChar().\n");
    }
    tcsetattr(0, TCSANOW, &oldChars);  // Setting old terminal settings.

}

// This function clears the user's input.
// It clears the input temporary buffer.
// It clears the input strings list.
// It clears (if present) the char* finalinput.
// It also clears the user's STDIN buffer, for that an unblocking getchar() was needed.
// It returns 1 if at least one char has been removed from STDIN, 0 otherwise.
char clearInput(void) {

    // Resetting temporary input buffer.
    memset(input, '\0', BUFFER_SIZE);

    // Destroying the strings list.
    destroyStringList();

    // Clearing char* finalinput.
    if (inputfinal != NULL) {
        free(inputfinal);
        inputfinal = NULL;
    }

    // Clearing STDIN buffer.
    char cleared = 0;
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

// This function substitute all the '\n' with '\0' in the input. 
// It returns 1 if at least a '\n' is found in the input, 0 otherwise.
char sanitizeCheckInput(void){

    struct StringNode* current = heads;
    char validinputlastline = 0;

    while (1) {
        if (current == NULL) break;
        char* currentstr = current->s;
        while(currentstr[0] != '\0'){
            if (currentstr[0] == '\n') {
                // An additional check.
                // The new line must be in the last input line read.
                if (current->n == NULL) validinputlastline = 1;
                currentstr[0] = '\0';
            }
            currentstr++;
        }
        current = current->n;
    }

    return validinputlastline;

}

// This function insert a string in strings list.
void insertStringInList(void) {

    // Creating the new element.
    struct StringNode* new;
    new = (struct StringNode*) malloc(sizeof(struct StringNode));
    if (new == NULL) {
        // Error
        handleError(0, 1, 0, 0, "Error in malloc() of the new strings list element in the insertStringInList().\n");
    }
    new->n = NULL;

    new->s = (char*) malloc(sizeof(char) * (BUFFER_SIZE + 1));
    if (new->s == NULL) {
        // Error
        handleError(0, 1, 0, 0, "Error in malloc() of the new string in the strings list element in the insertStringInList().\n");
    }
    // Copying current temporary line buffer in the string element list.
    strcpy(new->s, input);
    // Setting string terminator.
    uli l = strlen(input);
    new->s[l] = '\0';

    // Adding the new element to the strings list.
    if (heads == NULL) {
        // Empty list.
        heads = new;
    }else {
        // Not empty list.
        struct StringNode* current = heads;
        while (1) {
            if (current->n == NULL) break;
            current = current->n;
        }
        current->n = new;
    }

}

// This function check if there is/are some server's response/s to print.
// It handles the various cases and decides what to print or not to print using the
// previously setted var printprompt.
void checkResponses(void) {

    // There is/are something to print only if the messages list is not empty (head == NULL).
    char somethingtoprint = head == NULL ? 0 : 1;

    if (somethingtoprint == 1) {

        if (printprompt == 2) {
            // The input is not completed, clearing the input. Maybe there is 
            // a situation like this:
            // -> USERINPUT...
            clearInput();
            printff(NULL, 0, "\n");
        }

        if (printprompt == 3) {
            // Two cases:
            // 1. ->
            // 2. -> USERINPUT...

            char i = clearInput();
            if (i == 0) {
                // First case.
                printff(NULL, 0, "\n");
            }else {
                // Second case.
                printff(NULL, 0, "\n");
            }
            // No difference.
            // But for readability better to distinguish them.

        }

        if (printprompt == 1) {
            ;

            // Nothing to do.
            // But for readability better to distinguish.
        }

        // Here the server's response/s is/are printed.
        printResponses();

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

// This function process a completed user's input.
void processInput(void) {

       // ALL lines readed.
       // Completed input.

       // Converting input from list to static array.

       // Allocating a new input, but this will not a buffer, this will content exactly 
       // the input, without wasting space.

        // Removing (if present) the current char* inputfinal.
       if (inputfinal != NULL) free(inputfinal);

       // Counting the total numbers of characters of all the strings in the list.
       struct StringNode* current = heads;
       uli totalchars = 0LU;
       while(1) {
            if (current == NULL) break;
            totalchars += strlen(current->s);
            current = current->n;
       } 

       // Allocating memory for the final string input.
       inputfinal = (char*) malloc(sizeof(char) * ++totalchars);
       if (inputfinal == NULL) {
            // Error
            handleError(0, 1, 0, 0, "Error in malloc() of the processInput().\n");
       }

       current = heads;
       uli counter = 0LU;
       while(1) {
            if (current == NULL) break;
            char* s = current->s;
            // Copying char by char from string list element to the final char* finalinput.
            while(s[0] != '\0') {
                inputfinal[counter++] = s[0];
                s++;
            }
            inputfinal[counter] = s[0];
            current = current->n;
       }      
       // Manually settings the string terminator.
       inputfinal[totalchars] = '\0';

        // Destroying the string list.
        destroyStringList();

       // Now, after all this effort, we have the user input in "input[]" var and
       // we are ready to process it.

        // Normalize to lowercase the input.
        toLowerOrUpperString(inputfinal, 'L');

        // Processing the user request.
        if (strcmp("end", inputfinal) == 0 || strcmp("exit", inputfinal) == 0){
            printff(NULL, 0, "Bye, bye, see you soon! Thanks for playing.\n");
            sendMessage(client_fd, MSG_ESCI, NULL);
            pthread_exit(NULL);
            return;
        }
        if (strcmp("help", inputfinal) == 0) {
            printff(NULL, 0, HELP_MSG);
            return;
        }
        // TODO control sendMessages return -1.
        if (strcmp("matrix", inputfinal) == 0) {
            void* ret = sendMessage(client_fd, MSG_MATRICE, NULL);
            if (ret == (void*)-1L) {

            }else if (ret == NULL){
                //handleError("WARNING: a sendMessage() returned NULL. Nothing has been sent.\n")
            }else if (ret == (void*)-2L) {

            }else{

            }
            return;
        }

        // Tokenizing using a space (' ') the user input.
        // I don't care to destroy the string, since will be no used in the future.
        char* firstword = strtok(inputfinal, " ");
        if (firstword != NULL) {
            char* secondword = strtok(NULL, " ");
            if (secondword != NULL) {
                if (strcmp("register_user", firstword) == 0 && secondword != NULL) {
                    sendMessage(client_fd, MSG_REGISTRA_UTENTE, secondword);
                    return;
                }
                if (strcmp("p", firstword) == 0 && secondword != NULL) {
                    sendMessage(client_fd, MSG_PAROLA, secondword);
                    return;
                }
            }
        }

        printff(NULL, 0, "Unknown command. Use 'help' to know the available options.\n");
        return;


}

// ANCHOR inputHandler()
// This functions handles the user's input.
void inputHandler(void) {

    // Initializing is needed (to set the read() timeout).
    clearInput();

    while (1){

        // The part that follows is the reading of the user input performed in a VERY VERBOSE
        // and complicated way. It would have been much simpler to allocate a static fixed-length buffer.
        // But in this way we will have 2 problems, the first is that if the user input size is 
        // less than of the fixed size buffer, we will waste memory space. The second problem is 
        // the opposite, if the user input size is greater than the fixed size buffer, we will
        // not able to handle the entire input. This leads us (to avoid this last unsolvable 
        // problem) to allocate a very large buffer, and this makes the previous point worse and worse.
        // Instead, with the following code, we will dynamically allocate only the space
        // needed to handle the input of arbitrary length. 

        /*
        
        The main problem, is not knowing the length of the user input before space allocation.
        A possible simpler way might be to read character by character the input to the end
        (e.g., with a getchar()), count the characters, allocate the required space, and 
        finally start over from the beginning of the input (perhaps with a seek() or rewind()) 
        to copy it to the allocated destination. However, reading on the internet, I learned that it is
        impossible to restart reading the standard input safely on every platforms, there are some ways,
        but the result is not always guaranteed.

        So my solution was to dynamically save the content in a string list, containing each element, 
        a temporarily buffer of fixed size, while counting its length and afterwards
        allocate the total space required, copy the input from the list, and then destroy the list.
        Each line length will be maximum BUFFER_SIZE. If the user input is greater than
        BUFFER_SIZE, it will be splitted in more lines (more elements of the strings list).
        
        */

        // TL;DR: Solves the problem of not knowing the length of user input.
        

        // Printing prompt.
        if (printprompt == 1)
            printff(NULL, 0, PROMPT_STR);
        else printprompt = 0;

        while (1) {
                
            // Executed to read MULTIPLE lines.

            // Waiting for the user's input.
            // After this time the read() is automatically interrupted to check if there
            // are server's responses to print.
            // However, it is taken back and the user does not notice it.
            usleep(CHECK_RESPONSES_MICROSECONDS);

            // Reading from STDIN the user input.
            int retvalue;
            retvalue = read(STDIN_FILENO, input, BUFFER_SIZE);
            input[BUFFER_SIZE] = '\0';

            // Inserting the new input in the strings list.
            if (retvalue > 0) insertStringInList();
 
            if (retvalue != -1) {
                    // Something has been read (r > 0) or r == 0.

                    // MAY or MAY NOT have read ALL the data that the user would want to.
                    // The user may have entered LESS data but with '\n', so it's completed,
                    // OR the input is NOT completed ('\n' missing).
                    
                    // Checking if the input contains a '\n', to detect if it is completed or not,
                    // to know if it's necessary or not printing the newline and prompt and 
                    // to process the input.
                    if (sanitizeCheckInput() == 1) {
                        // The input is completed, we can process it.
                        // Processing...

                        processInput();

                        printprompt = 1;
                    }else {
                        // The input is not completed, clearing the input. Maybe there is 
                        // a situation like this:
                        // -> USERINPUT...

                        // To know what to do (printing and clearing input), we need to know 
                        // whether there are server's responses to print or not.
                        // So we defer this task to the function checkResponses(), setting this flag.
                        printprompt = 2;

                    }
            }else{
                // -1.
                // EAGAIN == 35 on macos
                // errno == 11 on linux
                if (errno == 35 || errno == 11){
                    // Normal interrupt to display server's responses received.
                    // Two cases:
                    // 1. ->
                    // 2. -> USERINPUT...

                    // To know what to do (printing and clearing input), we need to know 
                    // whether there are server's responses to print or not.
                    // So we defer this task to the function checkResponses(), setting this flag.
                    printprompt = 3;

                }else {
                    // Unexpected strange error.
                    handleError(1, 1, 0, 0, "Unexpected error while handling the user's input.\n");
                }
            }
            // Checking for server's responses.
            checkResponses();
            // Printing prompt is needed.
            if (printprompt == 1) break;

        } // End first while.

        // To remember the second while.
       continue;

    }


}

// ANCHOR responsesHandler()
// This function will be executed in a dedicated thread started after the socket connection in the main.
// It will run forever (as long as the process lives) looping in a while(1);.
// It will handle asynchronously all the responses received from the server to our requests,
// by adding them to a list of messages.
void* responsesHandler(void* args) {

    printff(NULL, 0, "I'm the responsesHandler() thread (ID): %lu.\n", (uli) responses_thread);
    // To setup the thread destructor.
    threadSetup();   
    setupfinished++;

    struct Message* received = NULL;

    while (1){

       // Wait to receive a message.
       received = receiveMessage(client_fd); 

       if ((long) received == -1L) {
            // Error
            // Probably disconnection.
            handleError(0, 1, 0, 0, "Probably disconnected by the server.\n");
       }

       if (received == NULL) continue;

        // Allocating a new heap element.
        struct MessageNode* new;
        new = (struct MessageNode*) malloc(sizeof(struct MessageNode));
        if (new == NULL) {
            // Error
            handleError(0, 1, 0, 0, "Error in allocating heap memory for a new received message for the list.\n");
        }

        // Filling the new element.
        new->next = NULL;
        new->m = received;

        mLock(&listmutex);
        // Adding to the list.
        if (head == NULL) {
            // List empty.
            head = new;
        }else{
            // Going through the list to add at the end the new element.
            struct MessageNode* c = head;
            while (1) {
                if (c->next == NULL) break;
                c = c->next;
            }
            c->next = new;
        }  
        mULock(&listmutex);

        continue;

    }

    return NULL;
    
}

void threadDestructor(void* args) {

    // TODO threadDestrucotr()
    if (pthread_self() == responses_thread) {

    }else if(pthread_self() == sig_thr_id){

    }else if(pthread_self() == mainthread){
        // Delegate to the atExit(), executed after the returns of this function.
        exit(EXIT_SUCCESS);
    }else{
        // Error
        handleError(0, 1, 0, 0, "Error, this thread shouldn't exist!\n");
    }


}

// This function is registered by the main thread with atexit().
// Will be executed before exiting by the main thread.
void atExit(void) {

    // TODO atExit()
    printff(NULL, 0, "EXIT!\n");

}

// ANCHOR signalsThread()
// This function will be executed in a dedicated thread started as soon as possible in the main.
// It will run forever (as long as the process lives) looping in a while(1);.
// Will only deal with the management of SIGINT, SIGPIPE signals, but
// all others could also be added without any problems.
// The SIGINT is sent when CTRL + C are pressed on the keyboard.
// All other threads will block these signals.
// This way should be better then registering the signal handler with the sigaction().
// Because we won't be interrupted and thus solve the very annoying problem of using
// async-safe-signals functions (reentrant functions).
// Note that still the problem of inter-thread competition persists and needs to be handled.
void* signalsThread(void* args) {

    printff(NULL, 0, "I'm the signalsThread() thread (ID): %lu.\n", (uli) sig_thr_id);
    // To setup the thread destructor.
    threadSetup();
    setupfinished++;

    int sig;    
    int retvalue;
    while (1){
        // Intercepting signals previously setted in the mask in the main.
        // sigwait() atomically get the bit from the signals pending mask and set it to 0.
        // https://stackoverflow.com/questions/2575106/posix-threads-and-signals
        retvalue = sigwait(&signal_mask, &sig);

        if (retvalue != 0) {
            // Error
            handleError(0, 0, 0, 0, "WARNING: Error in sigwait(). Trying to continue by ignoring the signal.\n");
            continue;
        }

        // Treatment of different signals.
        switch (sig){
            case SIGINT:{ 
                // TODO SIGINT
                break;
            }case SIGPIPE:{
                // Nothing, already handled by the single threads.
                break;
            }default:{
                // TODO Unexpected signal
                break;
            }
        }
    
    }

    
    return NULL;

}

// This function when called process a list of messages received from the server.
// It prints them and it removes them from the messages list.
void printResponses(void) {

    mLock(&listmutex);
    struct MessageNode* current = head;
    while(1) {
        if (current == NULL) break;

        struct Message* received = current->m;

        // Printing the server's responses based on the message type.
        switch (received->type){
            case MSG_MATRICE: {
                printff(NULL, 0, "%s", received->data);
                break;
            }case MSG_OK:{
                printff(NULL, 0, "%s", received->data);
                break;
            }case MSG_ERR: {
                printff(NULL, 0, "%s", received->data);
                break;
            }case MSG_TEMPO_ATTESA: {
                printff(NULL, 0, "The game is in pause. Seconds left to the end of the pause: %lu.\n", strtoul(received->data, NULL, 10));
                break;
            }case MSG_TEMPO_PARTITA: {
                printff(NULL, 0, "The game is ongoing. Seconds left to the end of the game: %lu.\n", strtoul(received->data, NULL, 10));
                break;
            }case MSG_PUNTI_PAROLA: {
                uli p = strtoul(received->data, NULL, 10);
                if (p == 0LU)
                    printff(NULL, 0, "Word already claimed. You got %lu points.\n", p);
                else
                    printff(NULL, 0, "Word claimed succesfully, nice guess! You got %lu points.\n", p);
                break;
            }case MSG_PUNTI_FINALI: {
                mLock(&mutexprint);
                printff(NULL, 1, "The game is over, this is the scoreboard.\n");
                char* tmp = received->data;
                while (1) {
                    if (tmp == received->data) tmp = strtok(tmp, ",");
                    else tmp = strtok(NULL, ",");
                    if (tmp == NULL) break;
                    printff(NULL, 1, "Name: %s. ", tmp);
                    tmp = strtok(NULL, ",");
                    printff(NULL, 1, "Points: %s.\n", tmp);
                }
                mULock(&mutexprint);
                break;
            }case MSG_ESCI : {
                // TODO Server disconnection alert.
                break;
            }case MSG_IGNORATO : {
                // TODO Ignored request.
                break;
            }case MSG_REGISTRA_UTENTE:
            case MSG_PAROLA: {
                // Error
                handleError(0, 0, 0, 0, "WARNING: Error, these functions are handled as in exit (sent from this client to the server), not as incoming (from the server to this client).\n");
                break;
            }default:
                // Error
                handleError(0, 0, 0, 0, "WARNING: Received an unknown server response, trying to continue by ignoring it!\n");
                break;
       }
        // Destroying message and element list.
       struct MessageNode* tmp;
       tmp = current->next;
       destroyMessage(&(current->m));
       free(current);
       current = tmp;
    }
    head = NULL;
    mULock(&listmutex);
    return;

}








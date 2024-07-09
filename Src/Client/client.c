// ANCHOR File begin.
// Shared client cross files vars and libs.
#include "client.h"

// The development of the input/output of this client was problematic, see the "./Studies/C/ClientInputOutputSync/inputoutputasynctests.c" file.

// Current file vars and libs.
#include <termios.h>

#define PROMPT_STR "[PROMPT BOGGLE]--> " // Prompt string.
#define PROMPT_STR_IT "[PROMPT PAROLIERE]--> " // Prompt string ITALIAN.

#define HELP_MSG "Avaible commands:\nhelp -> Show this page.\nregister_user user_name -> To register in the game.\nmatrix -> Get the current game matrix.\np word -> Submit a word.\nend -> Exit from the game.\n" // Help message.

#define CHECK_RESPONSES_MICROSECONDS 100 // Time in microseconds. Each time, the responses received from the server (if present), will be printed.
struct termios oldChars; // To save current terminal settings.
struct termios newChars; // To set new terminal settings.
int oldfl;
char printprompt; // This var is used to determine what to print and what not to print.

char* inputfinal;  // This heap allocated string will contain the full user input of arbitrary length.
char input[BUFFER_SIZE + 1]; // An input temporary buffer, we will handle arbitrary input length by using more BUFFER_SIZE strings in a list. +1 for the '\0'.
struct StringNode { // Support struct used to grab the user's input of arbitrary length in the client using a heap allocated linked list of char[BUFFER_SIZE + 1]. +1 for the '\0'.
    char s[BUFFER_SIZE + 1];   // Pointer to an heap allocated string.
    struct StringNode* n; // Pointer to the next list element.
};
struct StringNode* heads = NULL; // Head of the previous strings list.

struct MessageNode { // This will be a messages list. Will be filled asynchronously when a server's response is received by the responsesHandler() thread. The presence of the response in the list indicates that it has yet to be printed (shown to the user).
    struct Message* m; // Pointer to the Message struct.
    struct MessageNode* next; // Pointer to the next element of the list.
};
struct MessageNode* head = NULL; // Head of the previous messages list.
pthread_mutex_t listmutex = PTHREAD_MUTEX_INITIALIZER; // This mutex is used by the threads to synchronize on messages list operations.

// ANCHOR destroyStringList()
// This function destroys the strings list.
void destroyStringList(void) {

    struct StringNode* current = heads;
    while (1) {
        if (current == NULL) break;
        uli counter = 0LU;
        while (counter <= BUFFER_SIZE + 1) current->s[counter++] = '\0';
        struct StringNode* tmp = current->n;
        free(current);
        current = NULL;
        current = tmp;
    }
    heads = NULL;

}

// ANCHOR setUnblockingGetChar()
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

// ANCHOR setBlockingGetChar()
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

// ANCHOR clearInput()
// This function clears the user's input.
// It clears the input temporary buffer.
// It clears the input strings list.
// It clears (if present) the char* finalinput.
// It also clears the user's STDIN buffer, for that an unblocking getchar() was needed.
// It returns 1 if at least one char has been removed from STDIN, 0 otherwise.
char clearInput(void) {

    // Resetting temporary input buffer.
    uli counter = 0LU;
    while (counter <= BUFFER_SIZE + 1) input[counter++] = '\0';

    // Destroying the strings list.
    destroyStringList();

    // Clearing char* finalinput.
    free(inputfinal);
    inputfinal = NULL;

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

    return cleared;

}


// ANCHOR inputHandler()
// This functions handles the user's input.
void inputHandler(void) {

    printprompt = 1;

    while (1){

        // The part that follows is the reading of the user input performed in a VERY VERBOSE
        // and complicated way. It would have been much simpler to allocate a static fixed-length
        // buffer. But in this way we will have 2 problems, the first is that if the user input size 
        // is less than of the fixed size buffer, we will waste memory space. The second problem is 
        // the opposite, if the user input size is greater than the fixed size buffer, we will
        // not able to handle the entire input. This leads us (to avoid this last unsolvable 
        // problem) to allocate a very large buffer, and this makes the previous point worse.
        // Instead, with the following code, we will dynamically allocate only the space
        // needed to handle the input of arbitrary length. 

        /*
        
        The main problem, is not knowing the length of the user input before space allocation.
        A possible simpler way might be to read character by character the input to the end
        (e.g., with a getchar()), count the characters, allocate the required space, and 
        finally start over from the beginning of the input (perhaps with a seek() or rewind()) 
        to copy it to the allocated destination. However, reading on the internet, I learned that it is
        impossible to restart reading the standard input safely on every platforms, there are some
        ways, but the result is not always guaranteed.

        So my solution was to dynamically save the user's input in a strings list.
        Writing the read user's input in a temporarily buffer of fixed size (BUFFER_SIZE) in
        a strings element. If the BUFFER_SIZE space is not enough for all the user input,
        I create another strings list element, so I will have another array of more BUFFER_SIZE.
        When the user's input is finished, i will count its length and afterwards i will
        allocate the total space required, copy the input from the list, and then destroy the list.
        Each line length will be maximum BUFFER_SIZE. If the user's input is greater than
        BUFFER_SIZE, it will be splitted in more lines (more elements of the strings list).
        If the user's input is smaller than BUFFER_SIZE, I'm not going to waste a lot of space
        because I'm only going to use one item in the strings list.
        
        */

        // TL;DR: Solves the problem of not knowing the length of user input.
        
        // Periodically stop the read() and check for server's responses to print.
        // This is NOT about changing the mode of the getchar() function.
        // This MUST be done every while loop.
        int retvalue = fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
        if (retvalue == -1) {
            // Error
        }

        // Printing prompt.
        if (printprompt == 1)
            fprintf(stdout, PROMPT_STR_IT);
        else printprompt = 0;

        fflush(stdout);

        while (1) {
                
            // Executed to read MULTIPLE lines.

            // Waiting for the user's input.
            // After this time the read() is automatically interrupted to check if there
            // are server's responses to print.
            // if there are not, it is taken back and the user does not notice it.
            usleep(CHECK_RESPONSES_MICROSECONDS);

            // Reading from STDIN the user input.
            retvalue = read(STDIN_FILENO, input, BUFFER_SIZE);
 
            if (retvalue > 0) {

                    // Terminating the line.
                    input[retvalue] = '\0';

                    // Inserting the new input in the strings list.
                    // Creating the new element.
                    struct StringNode* new;
                    new = (struct StringNode*) malloc(sizeof(struct StringNode));
                    if (new == NULL) {
                        // Error
                    }
                    new->n = NULL;

                    // Copying current temporary line buffer in the string element list.
                    strcpy(new->s, input);
                    // Setting string terminator.
                    new->s[strlen(input)] = '\0';

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



                    // Something has been read r > 0.

                    // MAY or MAY NOT have read ALL the data that the user would want to.
                    // The user may have entered LESS data but with '\n', so it's completed,
                    // OR the input is NOT completed ('\n' missing).
                    
                    // Checking if the input contains a '\n', to detect if it is completed or not,
                    // to know if it's necessary or not printing the newline and prompt and 
                    // to process the input.
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



                    if (validinputlastline) {
                        // The input is completed, we can process it.
                        // Processing...

                        // ALL lines readed.
                        // Completed input.

                        // Converting input from list to heap allocated string.

                        // Allocating a new input, but this will not a buffer, this will content exactly 
                        // the input, without wasting space.

                        // Removing (if present) the current char* inputfinal.
                        free(inputfinal);
                        inputfinal = NULL;

                        // Counting the total numbers of characters of all the strings in the list.
                        struct StringNode* current = heads;
                        uli totalchars = 0LU;
                        while(1) {
                            if (current == NULL) break;
                            totalchars += strlen(current->s);
                            current = current->n;
                        } 

                        // Allocating memory for the final string input.
                        inputfinal = (char*) malloc(sizeof(char) * ++totalchars); // ++ for the '\0'.
                        if (inputfinal == NULL) {
                            // Error
                        }

                        current = heads;
                        uli counter = 0LU;
                        while(1) {
                            if (current == NULL) break;
                            char* s = current->s;
                            // Copying char by char from each strings list element
                            // to the final char* finalinput.
                             while(s[0] != '\0') {
                                inputfinal[counter++] = s[0];
                                s++;
                            }
                            current = current->n;
                        }      

                        // Manually settings the string terminator.
                        inputfinal[totalchars] = '\0';

                        // Destroying the string list.
                        destroyStringList();

                        // Now, after all this effort, we have the user input in inputfinal var and
                        // we are ready to process it.

                        // Normalize to lowercase the input.
                        toLowerOrUpperString(inputfinal, 'L');

                        char commandfound = 0;
                        // Processing the user request.
                        if (strcmp("end", inputfinal) == 0 || strcmp("exit", inputfinal) == 0 || strcmp("fine", inputfinal) == 0){
                            fprintf(stdout, "Bye, bye, see you soon! Thanks for playing.\n");
                            sendMessage(client_fd, MSG_ESCI, NULL);
                            commandfound = 1;
                            retvalue = pthread_cancel(responsesthread);
                            if (retvalue != 0) {
                                // Error
                            }
                            retvalue = close(client_fd);
                            if (retvalue == -1) {
                                // Error
                            }
                            exit(EXIT_SUCCESS);
                        }
                        if (strcmp("help", inputfinal) == 0 || strcmp("aiuto", inputfinal) == 0) {
                            fprintf(stdout, HELP_MSG);
                            commandfound = 1;
                        }
                        if (strcmp("matrix", inputfinal) == 0 || strcmp("matrice", inputfinal) == 0) {
                            sendMessage(client_fd, MSG_MATRICE, NULL);
                            commandfound = 1;
                            // TODO Control sendMessage() returns also in the server.
                        }
                        // Might be a command with at least two words.
                        // Tokenizing using a space (' ') the user's input.
                        // I don't care to destroy the string, since will be no used in the future.
                        char* firstword = strtok(inputfinal, " ");
                        if (firstword != NULL) {
                            char* secondword = strtok(NULL, " ");
                            if (secondword != NULL) {
                                toLowerOrUpperString(secondword, 'U');
                                if (strcmp("register_user", firstword) == 0 || strcmp("registra_utente", inputfinal) == 0 || strcmp("rg", inputfinal) == 0) {
                                    sendMessage(client_fd, MSG_REGISTRA_UTENTE, secondword);
                                    commandfound = 1;
                                }
                                if (strcmp("p", firstword) == 0) {
                                    sendMessage(client_fd, MSG_PAROLA, secondword);
                                    commandfound = 1;
                                }
                            }
                        }

                        if (!commandfound) fprintf(stdout, "Unknown command. Use 'help' to know the available options.\n");

                        // End processing.

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

                    // Unexpected error.

                }



            }

            // Checking for server's responses.
            // There is/are something to print only if the messages list is not empty (head == NULL).
            // We access read-only the global pointer to the first element of the messsages
            // list periodically, so although this can be read/write by the other thread,
            // it is not important.
            char somethingtoprint = head == NULL ? 0 : 1;

            if (somethingtoprint == 1) {

                if (printprompt == 2) {
                    // The input is not completed, clearing the input. Maybe there is 
                    // a situation like this:
                    // -> USERINPUT...
                    clearInput();
                    fprintf(stdout, "\n");
                }

                if (printprompt == 3) {

                    // Two cases:
                    // 1. ->
                    // 2. -> USERINPUT...

                    char i = clearInput();
                    if (i == 0) {
                        // First case.
                        fprintf(stdout, "\n");
                    }else {
                        // Second case.
                        fprintf(stdout, "\n");
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
                retvalue = pthread_mutex_lock(&listmutex);
                if (retvalue != 0) {
                    // Error
                }
                struct MessageNode* current = head;
                while(1) {
                    if (current == NULL) break;

                    struct Message* received = current->m;

                    // Printing the server's responses based on the message type.
                    switch (received->type){
                        case MSG_MATRICE: {
                            fprintf(stdout, "%s", received->data);
                            break;
                        }case MSG_OK:{
                            fprintf(stdout, "%s", received->data);
                            break;
                        }case MSG_ERR: {
                            fprintf(stdout, "%s", received->data);
                            break;
                        }case MSG_TEMPO_ATTESA: {
                            int t = atoi(received->data);
                            if (t != -1)
                                fprintf(stdout, "The game is in pause. Seconds left to the end of the pause: %lu.\n", strtoul(received->data, NULL, 10));
                            else
                                fprintf(stdout, "The game is in pause. We are late, the next game should start as soon as possible!\n");
                            break;
                        }case MSG_TEMPO_PARTITA: {
                            fprintf(stdout, "The game is ongoing. Seconds left to the end of the game: %lu.\n", strtoul(received->data, NULL, 10));
                            break;
                        }case MSG_PUNTI_PAROLA: {
                            uli points = strtoul(received->data, NULL, 10);
                            if (points == 0LU)
                                fprintf(stdout, "Word already claimed. You got %lu points.\n", points);
                            else
                                fprintf(stdout, "Word claimed succesfully, nice guess! You got %lu points.\n", points);
                            break;
                        }case MSG_PUNTI_FINALI: {
                            retvalue = pthread_mutex_lock(&printmutex);
                            if (retvalue != 0) {
                                // Error
                            }                            
                            fprintf(stdout, "The game is over, this is the scoreboard:\n");
                            char* tmp = received->data;
                            while (1) {
                                // First call to strtok().
                                if (tmp == received->data) tmp = strtok(tmp, ",");
                                // NOT first call.
                                else tmp = strtok(NULL, ",");
                                if (tmp == NULL) break;
                                fprintf(stdout, "Name: %s. ", tmp);
                                tmp = strtok(NULL, ",");
                                fprintf(stdout, "Points: %s.\n", tmp);
                                if (tmp == NULL) break;
                            }
                            retvalue = pthread_mutex_unlock(&printmutex);
                            if (retvalue != 0) {
                                // Error
                            }
                            break;
                        }case MSG_ESCI : {
                            // TODO Server disconnected you. Print the data message.
                            break;
                        }case MSG_REGISTRA_UTENTE:
                        case MSG_PAROLA: {
                            // Error
                            // Messages handled only by server.
                            break;
                        }default: {
                            // Error
                            break;
                        }
                    } // End switch.
                    // Destroying message and element list.
                    struct MessageNode* tmp;
                    tmp = current->next;
                    destroyMessage(&(current->m));
                    free(current);
                    current = NULL;
                    current = tmp;
                } // End while.
                head = NULL;
                retvalue = pthread_mutex_unlock(&listmutex);
                if (retvalue != 0) {
                    // Error
                }

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

                // For readability better to distinguish them.

            }

            // Printing prompt is needed.
            if (printprompt == 1) break;

        } // End first while.

        // To remember the second while.
       continue;

    }


}

// ANCHOR responsesHandler()
// This function will be executed in a dedicated thread started after the socket connection in the main.
// It will run forever (as long as the process lives) looping in a while(1).
// It will handle asynchronously all the responses received from the server to our requests,
// by adding them to a list of messages.
void* responsesHandler(void* args) {

    fprintf(stdout, "I'm the responsesHandler() thread (ID): %lu.\n", (uli) responsesthread);  

    int retvalue = pthread_mutex_lock(&setupmutex);
    if (retvalue != 0) {
        // Error
    }
    setupfinished++;
    pthread_setname_np(responsesthread, "ResponsesThread");
    retvalue = pthread_mutex_unlock(&setupmutex);
    if (retvalue != 0) {
        // Error
    }

    struct Message* received = NULL;

    while (1){
       // Wait to receive a message.
       char returncode;
       received = receiveMessage(client_fd, &returncode); 
        // This code could be:
        // - 0: A disconnection happened.
        // - 1: Nornally interrupted by a signal.
        // - 2: Unexpected error.
        // - 3: Read 0 bytes (at the message beginning, so the message's type).
        // - 4: Completed message received succesfully.
        // TODO Check returncode, the code below must be inserted in this switch in the fourth case.
        switch (returncode){
            case 0:
                /* code */
                break;
            
            default:
                break;
        }

        // Adding the new completed message received to list of messages (server responses to print).
        // Allocating a new heap element.
        struct MessageNode* new;
        new = (struct MessageNode*) malloc(sizeof(struct MessageNode));
        if (new == NULL) {
            // Error
        }

        // Filling the new element.
        new->next = NULL;
        new->m = received;

        retvalue = pthread_mutex_lock(&listmutex);
        if (retvalue != 0) {
            // Error
        }
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
        retvalue = pthread_mutex_unlock(&listmutex);
        if (retvalue != 0) {
            // Error
        }

        continue;

    }

    return NULL;
    
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

    fprintf(stdout, "I'm the signalsThread() thread (ID): %lu.\n", (uli) signalsthread);

    int retvalue = pthread_mutex_lock(&setupmutex);
    if (retvalue != 0) {
        // Error
    }
    setupfinished++;
    pthread_setname_np(signalsthread, "SignalsThread");
    retvalue = pthread_mutex_unlock(&setupmutex);
    if (retvalue != 0) {
        // Error
    }

    int sig;    
    while (1){
        // Intercepting signals previously setted in the mask in the main.
        // sigwait() atomically get the bit from the signals pending mask and set it to 0.
        // https://stackoverflow.com/questions/2575106/posix-threads-and-signals
        retvalue = sigwait(&signalmask, &sig);

        if (retvalue != 0) {
            // Error
            continue;
        }

        // Treatment of different signals.
        switch (sig){
            case SIGINT: { 
                // TODO SIGINT.
                fprintf(stdout, "CTRL + C intercepted!\n");
                exit(EXIT_SUCCESS);
                break;
            }case SIGPIPE: {
                // Nothing, already handled by the single threads.
                break;
            }default: {
                // Error
                // Unexpected signal.
                break;
            }
        }
    
    }
    
    return NULL;

}


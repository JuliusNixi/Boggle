// Shared client cross files vars and libs.
#include "client.h"

// Current file vars and libs.

#define PROMPT_STR "[PROMPT BOGGLE]--> " // Prompt string.
#define MSG_HELP "Avaible commands:\nhelp -> Show this page.\nregister_user user_name -> To register in the game.\nmatrix -> Get the current game matrix.\np word -> Submit a word.\nend -> Exit from the game.\n" // Help message.

// Support struct used to grab the user input of arbitrary length in the client using a
// heap allocated linked list of char* (these also heap allocated).
struct StringNode {
    char* s;   // Pointer to an heap allocated string.
    struct StringNode* n; // Pointer to the next list element.
};
struct StringNode* heads = NULL;
pthread_mutex_t mstringnode = PTHREAD_MUTEX_INITIALIZER;

// This will be a messages list.
// Will be filled asynchronously when a server response is received.
struct MessageNode {
    struct Message* m; // Pointer to the Message struct.
    struct MessageNode* next; // Pointer to the next element of the list.
};
struct MessageNode* head = NULL; // Head of the list.
pthread_mutex_t listmutex = PTHREAD_MUTEX_INITIALIZER; // This mutex is used by the threads to synchronize on list operations.

pthread_mutex_t mio = PTHREAD_MUTEX_INITIALIZER;
unsigned int receivedsignal = 0U;
unsigned int promptready = 0U;

pthread_t cleanerthread;
unsigned int completedprint = 0U;

unsigned int finalinputsize = 0U;
char* finalinput = NULL;
char input[BUFFER_SIZE]; // An input buffer, but in this way we will handle arbitrary input length by using more BUFFER_SIZE heap allocated strings.

char returnfromsignal[2] = "0";

// This function will be executed in a dedicated thread started as soon as possible in the main.
// It will run forever (as long as the process lives) looping in a while(1);.
// Will only deal with the management of SIGINT, SIGALRM, SIGPIPE signals, but
// all others could also be added without any problems.
// The SIGINT is sent when CTRL + C are pressed on the keyboard.
// All other threads will block these signals.
// This way should be better then registering the signal handler with the sigaction().
// Because we won't be interrupted and thus solve the very annoying problem of using
// async-safe-signals functions (reentrant functions).
// Note that still the problem of inter-thread competition persists and needs to be handled.
void* signalsThread(void* args) {

    int sig;    
    int retvalue;
    // TODO
    //threadSetup();
    while (1){
        // Intercepting signals previously setted in the mask in the main.
        // sigwait() atomically get the bit from the signals pending mask and set it to 0.
        // https://stackoverflow.com/questions/2575106/posix-threads-and-signals
        retvalue = sigwait(&signal_mask, &sig);

        if (retvalue != 0) {
            // Error
            handleError(0, 1, 0, 1, "Error in sigwait().\n");
        }

        // Treatment of different signals.
        switch (sig){
            case SIGINT:{ 
                // TODO
                break;
            }case SIGALRM:{
 
                mLock(&mio);
                // After prompt or user input i need to start a new line.
                printff(NULL, "\n");
                //printResponses();
                printff(NULL,"\n\nV\n\n");

                int retvalue;
                completedprint = 0U;
                retvalue = pthread_create(&cleanerthread, NULL, cleanerSTDIN, NULL);
                if (retvalue != 0){
                    // Error
                    // TODO
                }
                while (1){  
                    if (completedprint){
                        retvalue = pthread_cancel(cleanerthread);
                        if (retvalue != 0) {
                            // Error
                        }
                        break;
                    }
                }

                alarm(CHECK_RESPONSES_SECONDS);
                
                mULock(&mio);

                break;
            }case SIGPIPE:{
                // Nothing, already handled by the single threads.
                break;
            }default:{
                // TODO
                break;
            }
        }
    
    }

    
    return NULL;

}


// This function will be executed in a dedicated thread started after the socket connection in the main.
// It will run forever (as long as the process lives) looping in a while(1);.
// It will handle asynchronously all the responses received from the server to our requests,
// by adding them to a list of messages..
void* responsesHandler(void* args) {

    struct Message* received = NULL;

    while (1){
       
       // Wait to receive a message.
       received = receiveMessage(client_fd);  
  

       if ((long) received == -1L) {
            // Error
            // Probably disconnection.
            handleError(0, 1, 0, 1, "Probably disconnected by the server.\n");
       }

        // Allocating a new heap element.
        struct MessageNode* new;
        new = (struct MessageNode*) malloc(sizeof(struct MessageNode));
        if (new == NULL) {
            // Error
            handleError(0, 1, 0, 1, "Error in allocating heap memory for a new message of the list.\n");
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

    }

    return NULL;
    
}

// This function when called process a list of messages received from the server.
// It prints them and it remove them from the messages list.
void printResponses(void){

    mLock(&listmutex);
    struct MessageNode* c = head;

    while(1) {

        if (c == NULL) break;

        struct Message* received = c->m;
       // Printing the server response based on the message type.

       switch (received->type)
       {
        case MSG_MATRICE: {
            printff(NULL, "%s", received->data);
            break;
        }case MSG_OK:{
            printff(NULL, "%s", received->data);
            break;
        }case MSG_ERR: {
            printff(NULL, "%s", received->data);
            break;
        }case MSG_TEMPO_ATTESA: {
            printff(NULL, "The game is in pause. Seconds left to the end of the pause: %lu.\n", strtoul(received->data, NULL, 10));
            break;
        }case MSG_TEMPO_PARTITA: {
            printff(NULL, "The game is ongoing. Seconds left to the end of the game: %lu.\n", strtoul(received->data, NULL, 10));
            break;
        }case MSG_PUNTI_PAROLA: {
            unsigned int p = strtoull(received->data, NULL, 10);
            if (p == 0U)
                printff(NULL, "Word already claimed. You got %u points.\n", p);
            else
                printff(NULL, "Word claimed succesfully, nice guess! You got %u points.\n", p);
            break;
        }case MSG_PUNTI_FINALI: {
            printff(NULL, "The game is over, this is the scoreboard.\n");
            char* s = received->data;
            char* tmp = s;
            while (1) {
                if (tmp == received->data) tmp = strtok(s, ",");
                else tmp = strtok(NULL, ",");
                if (tmp == NULL) break;
                printff(NULL, "Name: %s. ", tmp);
                tmp = strtok(NULL, ",");
                printff(NULL, "Points: %s.\n", tmp);
            }
            break;
        }case MSG_ESCI : {
            // TODO   
            break;
        }case MSG_IGNORATO : {
            // TODO
            break;
        }default:
            // Error
            handleError(0, 0, 0, 0, "WARNING: Received an unknown server response, ignoring it!\n");
            break;
       }
       // Destroying message and element list.

       struct MessageNode* tmp;
       tmp = c->next;
       destroyMessage(&received);
       free(c);
       c = tmp;

    }

    mULock(&listmutex);

}

// This functions handles the user input.
void inputHandler(void) {

    while (1){

      // The part that follows is the reading of the user input performed in a VERY VERBOSE
      // and complicated way. It would have been much simpler to allocate a static fixed-length buffer.
      // But in this way we will have 2 problems, the first is that if the user input size is 
      // less than of the fixed size buffer, we will waste memory space. The second problem is 
      // the opposite, if the user input size is greater than the fixed size buffer, we will
      // not able to handle the entire input.
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
      allocate the total space required, copy the input from the lsit, and then destroy the list.
      Each line length will be maximum BUFFER_SIZE. If the user input is greater than
      BUFFER_SIZE, it will be splittend in more lines (more elements of the strings list).
      
      */

      // TL;DR: Solves the problem of not knowing the length of user input.
      
      // so is counter. It will rapresent the total final bytes to allocate on the heap to store
      // and process the user input.
      unsigned int so = 0U;
      unsigned int linescounter = 0U;
      int retvalue;
      unsigned int toexit = 0U;

      mLock(&mio);

      printff(NULL, PROMPT_STR);

      while (1) {
            
            // Executed to read MULTIPLE lines.

            // Reading from STDIN the user input.
            retvalue = read(STDIN_FILENO, input, BUFFER_SIZE);
            if (retvalue == -1 && errno != EINTR) {
                // Error
            }

            // Has been read (among these options):
            // 1. No interruption, the user input, last line or a normal line.
            // 2. Interruption, the user input, normal line (but first others may have been read)
            //    with retvalue > 0 and returnfromsignal == "1".
            // 3. Interruption, nothing (nothing inserted by the user for this line) 
            //   (but first others may have been read), retvalue == -1 && errno == EINTR.

            // 1. Nothing to do.

            // 2. 
            /*if (strcmp(returnfromsignal, "1") == 0 && retvalue > 0) {
                read(STDIN_FILENO, NULL, )
                toexit = 1U;
                break;
            }*/

            // 3.
            if (strcmp(returnfromsignal, "1") == 0 && retvalue == -1 && errno == EINTR) {
                toexit = 1U;
                break;
            }

            // New line.
            linescounter++;
            toexit = 0U;

            // Updating so with the current user input part read.
            // I need to allocate an extra byte to store '\0'.
            so += (sizeof(char) * ++retvalue);
            
            // Allocating heap space for the user input current part.
            char* s = (char*) malloc(sizeof(char) * retvalue);
            if (s == NULL) {
                // Error
                handleError(0, 1, 0, 1, "Error in allocating space for a string part of the user input.\n");
            }
            // Copying the part from the static buffer to dynamic allocated string.
            strcpy(s, input);
            // Terminating it.
            s[retvalue - 1] = '\0';
           
            // Allocating on the heap an element list and initializing it.
            struct StringNode* e = (struct StringNode*) malloc(sizeof(struct StringNode));
            if (e == NULL)  {
                // Error
                handleError(0, 1, 0, 1, "Error in allocating space for an ELEMENT part of the user input.\n");
            }
            e->s = s;
            e->n = NULL;

            // Going through the list to the end to add the new element at the end.
            struct StringNode* c = heads;
            if (heads == NULL){
                // Empty list.
                heads = e;
            }else {
                // Go until list end.
                while (1) {
                    if (c->n == NULL) break;
                    c = c->n;
                }
                c->n = e;
            }

            // New line detected, end of user input reached.
            // This is the last part, the user input is ended, i exit the while.
            if (s[retvalue - 2] == '\n') break;

        } // End While

        if (toexit){
            strcpy(returnfromsignal, "1");
            mULock(&mio);
            continue;
        }

        // ALL lines readed.

       // Allocating a new input, but this will not a buffer, this will content exactly 
       // the input, without wasting space.

       if (finalinput != NULL) free(finalinput);
       // Each line has a '\0' to be subtracted from the total.
       finalinputsize = (so - linescounter);
       finalinput = (char*) malloc(sizeof(char) * finalinputsize);
       if (finalinput == NULL) {
            // Error
            // TODO
       }

       unsigned int c = 0U; // Total counter.
       struct StringNode* ec = heads;
       unsigned int sc = 0U; // Line counter.
       while (1) {
          // End list.
          if (ec == NULL) break;
          // Copying the content of a string of the list in the "input[]".
          // Why not using already used strcpy? I don't now, i'm stupid (later comment during code review).
          sc = 0U;
          while (1) {
            if (ec->s[sc] == '\0') break;
            finalinput[c++] = ec->s[sc++];
          }
          ec = ec->n;
       }  
       // We have in input[c] == '\0';    
       // Fixing the previous '\n'. 
       // Now the string will be double terminated '\0\0'.
       finalinput[c - 1] = '\0';

        // Destroying the list.
       ec = heads;
       while (1){
            if (ec == NULL) break;
            free(ec->s);
            struct StringNode* tmp;
            tmp = ec->n;
            free(ec);
            ec = tmp;
       }
       heads = NULL;




       // Now, after all this effort, we have the user input in "input[]" var and
       // we are ready to process it.


   

        // Normalize to lowercase the input.
        toLowerOrUpperString(finalinput, 'L');

        printff(NULL,"INSERTED: %s\n",finalinput);

        // Processing the user request.

        if (strcmp("end", finalinput) == 0){
            printff(NULL, "Bye, bye, see you soon! Thanks for playing.\n");
            sendMessage(client_fd, MSG_ESCI, NULL);
            break;
        }
        if (strcmp("help", finalinput) == 0) {
            printff(NULL, MSG_HELP);
            mULock(&mio);
            continue;
        }
        if (strcmp("matrix", finalinput) == 0) {
            sendMessage(client_fd, MSG_MATRICE, NULL);
            mULock(&mio);
            continue;
        }

        // Tokenizing using a space (' ') the user input.
        // I don't care to destroy the string, since will be no used in the future.
        char* firstword = strtok(finalinput, " ");
        if (firstword != NULL) {
            char* secondword = strtok(NULL, " ");
            if (secondword != NULL) {
                if (strcmp("register_user", firstword) == 0 && secondword != NULL) {
                    sendMessage(client_fd, MSG_REGISTRA_UTENTE, secondword);
                    mULock(&mio);
                    continue;
                }
                if (strcmp("p", firstword) == 0 && secondword != NULL) {
                    sendMessage(client_fd, MSG_PAROLA, secondword);
                    mULock(&mio);
                    continue;
                }
            }
        }

        printff(NULL, "Unknown command. Use 'help' to know the available options.\n");
        mULock(&mio);
        continue;

    }


}

// This function is the SIGUSR1 signal handler.
// It executed by checkResponses() thread, when received the signal SIGUSR1 from signalsThread()
// which corresponds to check if there are server's responses to print.
void checkResponses(int signum) {

    /*
    
     This is used only to interrupt the read() with EINTR in errno. SCHERZAVO
     SHERZAVO

     */

    // strcpy() is Signal-Safe.
    // https://man7.org/linux/man-pages/man7/signal-safety.7.html
    strcpy(returnfromsignal, "1");

    return;

}

void* cleanerSTDIN(void* args){

    memset(input, '\0', BUFFER_SIZE);

    if (finalinput != NULL) {
        free(finalinput);
        finalinput = NULL;
    }

    struct StringNode* ec = heads;
    while (1){
        if (ec == NULL) break;
        free(ec->s);
        struct StringNode* tmp;
        tmp = ec->n;
        free(ec);
        ec = tmp;
    }
    heads = NULL;

    while (++completedprint && getchar() != '\0');
    return NULL;

}

void threadDestructor(void* args) {

    // TODO

}

// This function is registered by the main thread with atexit().
// Will be executed before exiting by the main thread.
void atExit(void) {

    // TODO

}

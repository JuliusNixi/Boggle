// Shared client cross files vars and libs.
#include "client.h"

// Current file vars and libs.
#define USAGE_MSG "Invalid args. Usage: ./%s server_ip server_port.\n" // Message to print when the user insert wrong args.
#define MSG_HELP "Avaible commands:\nhelp -> Show this page.\nregister_user user_name -> To register in the game.\nmatrix -> Get the current game matrix.\np word -> Submit a word.\nend -> Exit from the game.\n" // Help message.
#define PROMPT_STR "[PROMPT BOGGLE]--> " // Prompt string.

// Support struct used to grab the user input of arbitrary length in the client using a
// heap allocated linked list of char* (these also heap allocated).
struct StringNode {
    char* s;   // Pointer to an heap allocated string.
    struct StringNode* n; // Pointer to the next list element.
};

pthread_t responses_thread; // It will handle the responses received from the server asynchronously.

int main(int argc, char** argv) {

    // Initializing local vars.
    int retvalue = 0; // To check system calls result (succes or failure).

    // Initializing shared client cross vars.
    client_fd = 0; 

    // Shared/Common CLIENT & SERVER cross files vars and libs initialization.
    mainthread = pthread_self();

    // Printing banner.
    printf("\n\n##################\n#     CLIENT     #\n##################\n\n");

    // Creating a mask, that will block the SIGINT, SIGALRM and SIGPIPE signals for all 
    // threads except the dedicated thread signalsThread().
    // Important to do it as soon as possible.
    // Not to be caught unprepared.
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGINT);
    sigaddset(&signal_mask, SIGALRM);
    sigaddset(&signal_mask, SIGPIPE);

    // Registering exit function for main.
    retvalue = atexit(atExit);
    if (retvalue != 0) {
        // Error
        // errno
        handleError(1, 1, 0, 1, "Error in registering exit cleanupper main function.\n");
    }
    printff(NULL, "Exit safe function (cleanup for the main) registered correctly.\n");

    // Enabling the mask.
    retvalue = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
    if (retvalue != 0) {
        // Error
        handleError(0, 1, 0, 1, "Error in setting the pthread signals mask.\n");
    }
    printff(NULL, "Threads signals mask enabled correctly.\n");

    // Any newly created threads INHERIT the signal mask with these signals blocked. 

    // Creating the thread that will handle ALL AND EXCLUSIVELY SIGINT, SIGALRM and SIGPIPE signals
    // by waiting with sigwait() forever in loop.
    retvalue = pthread_create(&sig_thr_id, NULL, signalsThread, NULL);
    if (retvalue != 0) {
        // Error
        handleError(0, 1, 0, 1, "Error in creating the pthread signals handler.\n");
    }

    printff(NULL, "Signals registered and thread handler started succesfully.\n");

    // Check number of args.
    if (argc != 3) {
        // Error
        handleError(0, 1, 0, 1, USAGE_MSG, argv[0]);
    }

    // Parsing port.
    unsigned long int port = strtoul(argv[2], NULL, 10);
    if (port > 65535LU) {
        // Error
        handleError(0, 1, 0, 1, "Invalid port %lu. It should be less than 65535 and higher than 0.\n", port);
    }
    server_addr.sin_port = htons(port);

    server_addr.sin_family = AF_INET;

    // Parsing IP.
    retvalue = parseIP(argv[1], &server_addr);
    if (retvalue != 1) {
        // Error
        handleError(0, 1, 0, 1, "Invalid IP: %s.\n", argv[1]);
    }
    printf("Starting client on IP: %s and port: %lu.\n", argv[1], port);

    printf("The args seems to be ok...\n");

    // Creating socket.
reconnecting:
    client_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
    if (client_fd == -1) {
        // Error
        handleError(0, 1, 0, 1, "Error in creating socket.\n");
    }
    printf("Socket created.\n");

    // Connecting to server.
    while (1) {
        retvalue = connect(client_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        if (retvalue == -1){
            // On error of connect, the socket will be modified in a unspecified state.
            // So it will not be able to simply trying again a new connect.
            // Read here the notes.
            // https://man7.org/linux/man-pages/man2/connect.2.html
            // Need to recreate the socket...
            // Error
            // errno
            retvalue = close(client_fd);
            if (retvalue != 0) {
                // Error
                // errno
                handleError(1, 1, 0, 1, "Error in socket close(), during the failure of connect().\n");
            }
            printf("Error in connecting, retrying in 3 seconds.\n");
            sleep(3);
            goto reconnecting;
        }else break;
    }
    printf("Connected succesfully!\n");

    // Creating responses handler pthread.
    retvalue = pthread_create(&responses_thread, NULL, responseHandler, NULL);
    if (retvalue != 0) {
        // Error
        handleError(0, 1, 0, 1, "Error during the responses handler pthread creation.\n");
    }
    printf("Responses pthread created succesfully.\n");

    while (1){

      printf(PROMPT_STR);

      // The part that follows is the reading of the user input performed in a VERY VERBOSE
      // and complicated way. It would have been much simpler to allocate a static fixed-length buffer.
      // Instead, with the following code, we will dynamically allocate only the space
      // needed to handle the input of arbitrary length. 

      /*
      
      The problem, is not knowing the length of the user input before space allocation.
      A possible simpler way might be to read character by character the input to the end
      (e.g., with a getchar()), count the characters, allocate the required space, and 
      finally start over from the beginning of the input (perhaps with a seek() or rewind()) 
      to copy it to the allocated destination. However, reading on the internet, I learned that it is
      impossible to restart reading the standard input safely on every platforms, there are some ways,
      but the result is not always guaranteed.

      So my solution was to dynamically save the content in a string list,
      using only temporarily a buffer of fixed size, while counting its length and afterwards
      allocate the space required, copy the input, and then free and destroy the list.
      Each line length will be maximum BUFFER_SIZE. If the user input is greater than
      BUFFER_SIZE, it will be splittend in more lines (elements of the strings list).
      
      */

      // TL;DR: Solves the problem of not knowing the length of user input.
      struct StringNode* head = NULL;
      // so is counter. It will rapresent the total final bytes to allocate on the heap to store and process the user input.
      unsigned int so = 0U;
      unsigned int linescounter = 0U;
      while (1) {
            // New line.
            so = 0U;
            linescounter++;
            // An input buffer, but in this way we will handle arbitrary input length by using
            // more BUFFER_SIZE heap allocated strings.
            char input[BUFFER_SIZE];

            // Reading from STDIN the user input.
            retvalue = read(STDIN_FILENO, input, BUFFER_SIZE);
            if (retvalue == -1) {
                // Error
                handleError(1, 1, 0, 1, "Error in reading from STDIN.\n");
            }
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
            struct StringNode* c = head;
            if (head == NULL){
                // Empty list.
                head = e;
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

       // Allocating a new input, but this will not a buffer, this will content exactly 
       // the input, without wasting space.

       // Each line has a '\0' to be subtracted from the total.
       char input[so - linescounter];
       unsigned int c = 0U; // Total counter.
       struct StringNode* ec = head;
       unsigned int sc = 0U; // Line counter.
       while (1) {
          // End list.
          if (ec == NULL) break;
          // Copying the content of a string of the list in the "input[]".
          // Why not using already used strcpy? I don't now, i'm stupid (later comment during code review).
          sc = 0U;
          while (1) {
            if(ec->s[sc] == '\0') break;
            input[c++] = ec->s[sc++];
          }
          ec = ec->n;
       }  
       // In input[c] == '\0';    
       // Fixing the previous '\n'. 
       // Now the string will be double terminated '\0\0'.
       input[c - 1] = '\0';

        // Destroying the list.
       ec = head;
       while (1){
            if (ec == NULL) break;
            free(ec->s);
            struct StringNode* tmp;
            tmp = ec->n;
            free(ec);
            ec = tmp;
       }
       head = NULL;
       
       // Now, after all this effort, we have the user input in "input[]" var and
       // we are ready to process it.

        // Normalize to lowercase the input.
        toLowerOrUpperString(input, 'L');

        // Processing the user request.

        if (strcmp("end", input) == 0){
            printf("Bye, bye, see you soon! Thanks for playing.\n");
            sendMessage(client_fd, MSG_ESCI, NULL);
            break;
        }
        if (strcmp("help", input) == 0) {
            printf(MSG_HELP);
            continue;
        }
        if (strcmp("matrix", input) == 0) {
            sendMessage(client_fd, MSG_MATRICE, NULL);
            continue;
        }

        // Tokenizing using a space (' ') the user input.
        // I don't care to destroy the string, since will be no used in the future.
        char* firstword = strtok(input, " ");
        char* secondword = strtok(NULL, " ");
        if (strcmp("register_user", firstword) == 0 && secondword != NULL) {
            sendMessage(client_fd, MSG_REGISTRA_UTENTE, secondword);
            continue;
        }
        if (strcmp("p", firstword) == 0 && secondword != NULL) {
            sendMessage(client_fd, MSG_PAROLA, secondword);
            continue;
        }

        printf("Unknown command. Use 'help' to know the available options.\n");
        continue;

    }

    // TODO
    // Exit
/*
    // Chiusura del socket
    SYSC(retvalue, close(client_fd), "nella close");

*/

    return 0;
    
}




// Shared client files vars and libs.
#include "client.h"

// Current file vars and libs.

#define USAGE_MSG "Invalid args. Usage: ./%s nome_server porta_server.\n" // Message to print when the user insert wrong args.

#define MSG_AIUTO "Comandi disponibili:\naiuto -> Mostra questa pagina.\nregistra_utente nome_utente -> Permette di registrarsi nel gioco.\nmatrice -> Ottiene la corrente matrice di gioco.\np parola_indicata -> Invia una parola trovata.\nfine -> Esce dal gioco.\n"

// Support struct used to grab the user input of arbitrary length in the client.
struct StringNode {
    char* s;   // Pointer to an heap allocated string.
    struct StringNode* n; // Pointer to the next list element.
};

int main(int argc, char** argv) {

    int retvalue; // To check system calls result (succes or failure).

    // Creating a mask, that will block the SIGINT signal, and enabling
    // it for the current main thread.
    // Important to do it as soon as possible.
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGINT);
    retvalue = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
    if (retvalue != 0) {
        // Error
        printf("Error in setting the pthread signals mask.\n");
    }

    // Any newly created threads INHERIT the signal mask with these signals blocked. 

    // Creating the thread that will handle ALL AND EXCLUSIVELY SIGINT by
    // waiting with waitsignal() forever.
    retvalue = pthread_create(&sig_thr_id, NULL, signalsThread, NULL);
    if (retvalue != 0) {
        // Error
        printf("Error in creating the pthread signal handler.\n");
    }

    printf("Signal registered and thread handler started succesfully.\n");

    // Initializing shared vars.
    client_fd = -1;
    // PTHREAD_MUTEX_INITIALIZER only available with statically allocated variables.
    // In this case i must use pthread_mutex_init().
    retvalue = pthread_mutex_init(&(mprint), NULL);
    if (retvalue != 0) {
        // Error
        printf("Error in mutex printing initialization.\n");
    }
    aprint = 0;

    // Printing banner.
    printf("\n\n##################\n#     CLIENT     #\n##################\n\n");

    // Check number of args.
    if (argc != 3) {
        // Error
        printf(USAGE_MSG, argv[0]);
    }

    // Registering exit functions.
    retvalue = atexit(clearExit);
    if (retvalue == -1) {
        // Error
        printf("Error during the registration of the safe function exit (cleanup).\n");
    }
    printf("Exit safe exit function (cleanup) registered correctly.\n");

    // Parsing port.
    unsigned int port = atoi(argv[2]);
    if (port > 65535U || (int) port <= 0U) {
        // Error
        printf("Invalid port %u. It should be less than 65535 and higher than 0.\n", port);
    }
    server_addr.sin_port = htons(port);

    server_addr.sin_family = AF_INET;

    // Parsing IP.
    retvalue = parseIP(argv[1], &server_addr);
    if (retvalue != 1) {
        // Error
        printf("Invalid IP: %s.\n", argv[1]);
    }

    printf("Starting client on IP: %s and port: %u.\n", argv[1], port);

    printf("The args seems to be ok...\n");

    // Creating socket.
    client_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
    if (client_fd == -1) {
        // Error
        printf("Error in creating socket.\n");
    }
    printf("Socket created.\n");

    // Connecting to server.
    while (1) {
        retvalue = connect(client_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        if (retvalue == -1){
            // Error
            printf("Error in connect, retrying in 3 seconds.\n");
            sleep(3);
        }else break;
    }
    printf("Connected succesfully!\n");

    // Creating responses handler thread.
    pthread_t responses_thread;
    retvalue = pthread_create(&responses_thread, NULL, responseHandler, NULL);
    if (retvalue != 0) {
        // Error
        printf("Error during the responses handler thread creation.\n");
    }
    printf("Responses thread created succesfully.\n");

    while (1){

       // Printing prompt and avoiding the temporary printing of responses to previously made
       // requests.
       mLock(&mprint);
       if (aprint == 0) {
            fflush(stdin);
            while (1) if ((retvalue = read(STDIN_FILENO, NULL, 1)) == 0) break;
            printf(PROMPT_STR);
            fflush(stdout);
            // Prompt printed. Informing the other thread (responses handler) to not print it.
            aprint = 1;
       }
       mULock(&mprint);

      // The part that follows is the reading of the user input performed in a VERY VERBOSE
      // and complicated way. It would have been much simpler to allocate a static fixed-length buffer.
      // Instead, with the following code, we will dynamically allocate only the space
      // needed to handle the input. 

      /*
      
      The problem, is not knowing the length of the user input before space allocation.
      A possible simpler way might be to read character by character the input to the end
      (e.g., with a getchar()), count the characters, allocate the required space, and 
      finally start over from the beginning of the input (perhaps with a seek()) to copy it
      to the allocated destination. However, reading on the internet, I learned that it is
      impossible to restart reading the standard input safely, there are some ways, but the
      result is not always guaranteed.

      So my solution was to dynamically save the content in a string list,
      using only temporarily a buffer, while counting its length and afterwards
      allocate the space required, copy the input, and then free and destroy the list.
      
      */

      // TL;DR: Solves the problem of not knowing the length of user input.
      struct StringNode* head = NULL;
      // so is counter. It will rapresent the total bytes to allocate on the heap to store and process the user input.
      int so = 0;
      while (1) {
            // An input buffer.
            char input[BUFFER_SIZE];

            // Reading from STDIN the user input.
            retvalue = read(STDIN_FILENO, input, BUFFER_SIZE);
            if (retvalue == -1) {
                // Error
                printf("Error in reading from STDIN.\n");
            }
            // Updating so with the current user input part read.
            so += (sizeof(char) * retvalue);

            // If the last char is NOT a new line, the user input read
            // filled the whole buffer and it was NOT ENOUGH.
            // So i need to allocate an extra byte to store '\0'
            // and understand where a part in the list will finish.
            char f = input[retvalue - 1];
            if (f != '\n') retvalue++;
            
            // Allocating heap space for the user input current part.
            char* s = (char*) malloc(sizeof(char) * retvalue);
            if (s == NULL) {
                // Error
                printf("Error in allocating space for a STRING part of the user input.\n");
            }
            // Copying the part from the stati buffer to dynamic string.
            strcpy(s, input);
            // Terminating it.
            s[retvalue - 1] = '\0';
            
            // Allocating on the heap an element list and initializing it.
            struct StringNode* e = (struct StringNode*) malloc(sizeof(struct StringNode));
            if (e == NULL)  {
                // Error
                printf("Error in allocating space for an ELEMENT part of the user input.\n");
            }
            e->s = s;
            e->n = NULL;

            // Going through the list to the end to add the new element at the end.
            struct StringNode* c = head;
            if (head == NULL){
                head = e;
                c = head;
            }else {
                while (1) {
                    if (c->n == NULL) break;
                    c = c->n;
                }
                c->n = e;
                if (c->n != NULL) c = c->n;
            }
            // Terminating the current string user input part.
            (c->s)[retvalue - 1]  = '\0';

            // New line, this is the last part, the user input is ended, i exit the while.
            if (f == '\n') break;

        } // End While

       // Allocating a new input, but this will not a buffer, this will content exactly 
       // the input, without wasting space.
       char input[so];
       int c = 0;
       struct StringNode* ec = head;
       while (1) {
          // End list.
          if (ec == NULL) break;
          // Copying the content of a string of the list in the "input[]".
          int sc = 0;
          while (1) {
            input[c++] = ec->s[sc++];
            if (ec->s[sc - 1] == '\0') break;
          }
          ec = ec->n;
       }

       // Now we have the user input in "input[]" var and we are ready to process it.

        // Normalize to lowercase the input.
        toLowerOrUpperString(input, 'L');

        // Processing the user request.
        // aprint = 0, means someone should print again the prompt.

        if (strcmp("fine", input) == 0){
            mLock(&mprint);
            printf("Ciao, ciao, alla prossima! Grazie per aver giocato.\n");
            sendMessage(client_fd, MSG_ESCI, NULL);
            // I quit the program, no need to release the lock.
            break;
        }
        if (strcmp("aiuto", input) == 0) {
            mLock(&mprint);
            printf(MSG_AIUTO);
            aprint = 0;
            mULock(&mprint);
            continue;
        }
        if (strcmp("matrice", input) == 0) {
            mLock(&mprint);
            sendMessage(client_fd, MSG_MATRICE, NULL);
            aprint = 0;
            mULock(&mprint);
            continue;
        }

        // Tokenize using a space (' ') the user input.
        char* firstword = strtok(input, " ");
        char* secondword = strtok(NULL, " ");
        if (strcmp("registra_utente", firstword) == 0 && secondword != NULL) {
            mLock(&mprint);
            sendMessage(client_fd, MSG_REGISTRA_UTENTE, secondword);
            aprint = 0;
            mULock(&mprint);
            continue;
        }
        if (strcmp("p", firstword) == 0 && secondword != NULL) {
            mLock(&mprint);
            sendMessage(client_fd, MSG_PAROLA, secondword);
            aprint = 0;
            mULock(&mprint);
            continue;
        }

        mLock(&mprint);
        printf("Comando non valido. Usa il comando 'aiuto' per sapere le opzioni disponibili.\n");
        aprint = 0;
        mULock(&mprint);

        continue;

    }

    // Exit
/*
    // Chiusura del socket
    SYSC(retvalue, close(client_fd), "nella close");

*/

    return 0;
    
}




// Shared client files vars and libs.
#include "client.h"

// Current file vars and libs.
// ...Nothing...

// This function will be executed in a dedicated thread started as soon as possible in the main.
// It will run forever (as long as the process lives) looping in a while(1);.
// Will only deal with the management of SIGINT signal, but
// all others could also be added without any problems.
// The SIGINT is sent when CTRL + C are pressed on the keyboard.
// All other threads will block this signal.
// This way should be better then registering the signal handler with the sigaction().
// Because we won't be interrupted and thus solve the very annoying problem of using
// async-safe-signals functions (reentrant functions).
// Note that still the problem of inter-thread competition persists and needs to be handled.
void* signalsThread(void* args) {

    // TODO
    return NULL;

}

// This function is a cleanupper called before the exit of the program.
void clearExit(void){

    // TODO
    return;

}

// This function will be executed in a dedicated thread started after the socket connection in the main.
// It will run forever (as long as the process lives) looping in a while(1);.
// It will handle asynchronously all the responses received from the server to out requests.
void* responseHandler(void* args) {

    struct Message* received = NULL;

    while (1){
       
       // Wait to receive a message.
       received = receiveMessage(client_fd);

       mLock(&mprint);

        // Check if printing prompt is needed.
        if (aprint == 0) {
            fflush(stdin);
            printf(PROMPT_STR);
            fflush(stdout);
            aprint = 1;
        }

       printf("\n");

        // Processing and printing the server response.
       switch (received->type)
       {
        case MSG_MATRICE: {
            printf("%s", received->data);
            break;
        }case MSG_OK:{
            printf("%s", received->data);
            break;
        }case MSG_ERR: {
            printf("%s", received->data);
            break;
        }case MSG_TEMPO_ATTESA: {
            printf("The game is in pause. Seconds left to the end of the pause: %d.\n", atoi(received->data));
            break;
        }case MSG_TEMPO_PARTITA: {
            printf("The game is ongoing. Seconds left to the end of the game: %d.\n", atoi(received->data));
            break;
        }case MSG_PUNTI_PAROLA: {
            int p = atoi(received->data);
            if (p == 0)
                printf("Word already claimed. You got %d points.\n", p);
            else
                printf("Word claimed succesfully, nice guess! You got %d points.\n", p);
            break;
        }case MSG_IGNORATO: {
            printf("Le eventuali richieste INCOMPLETE trasmesse sono state ignorate a causa della fine del gioco.\n");
            break;
        }default:
            // Error
            printf("Error, received an unknown server response!\n");
            break;
       }
       
       fflush(stdin);
       printf(PROMPT_STR);
       fflush(stdout);
       aprint = 1;
       mULock(&mprint);

       destroyMessage(&received);

    }
    

}




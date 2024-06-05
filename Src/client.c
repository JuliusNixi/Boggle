// Shared client cross files vars and libs.
#include "client.h"

// Current file vars and libs.

// This will be a messages list.
// Will be filled asynchronously when a server response is received.
struct MessageNode {
    struct Message* m; // Pointer to the Message struct.
    struct MessageNode* next; // Pointer to the next element of the list.
};
struct MessageNode* head = NULL; // Head of the list.
pthread_mutex_t listmutex = PTHREAD_MUTEX_INITIALIZER; // This mutex is used by the threads to synchronize on list operations.

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
                // TODO
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
void* responseHandler(void* args) {

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
void printResponse(void){

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

void threadDestructor(void* args) {

    // TODO

}

// This function is registered by the main thread with atexit().
// Will be executed before exiting by the main thread.
void atExit(void) {

    // TODO

}

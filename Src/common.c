#include "common.h"

// This function parse an IP and return the result code. If different by 1, there was an error.
// It also execute a inet_aton on a struct sockaddr_in.
int parseIP(char* ip, struct sockaddr_in* sai) {

    if (ip == NULL || sai == NULL) {
        // Error
        printf("Error, parseIP() received a NULL ip or sockaddr_in.\n");
    }

    // Lowering, LoCalhost and localhost and LOCALHOST, are now identical.
    toLowerOrUpperString(ip, 'L');
    int retvalue;
    // Converting localhost to 127.0.0.1.
    if (strcmp(ip, "localhost") == 0)
        retvalue = inet_aton("127.0.0.1", &sai->sin_addr);
    else
        retvalue = inet_aton(ip, &sai->sin_addr);
    return retvalue;

}

// This function normalize a string trasforming it in lower/upper case.
// 'L' means lowercase, any other upper and is specified by lowerupper.
void toLowerOrUpperString(char* string, char lowerupper) {

    if (string == NULL) {
        // Error
        printf("Error, toLowerOrUpperString() received an empty string.\n");
    }
    // Checking if lowerupper is 'L' (means to change the string in lowercase),
    // or 'U' (means to change the string in UPPERCASE).
    lowerupper = toupper(lowerupper);
    if (lowerupper != 'L' && lowerupper != 'U') {
            // Error
            printf("Error in toLowerOrUpperString(), the second arg must be 'L' for lower or 'U' for upper.\n");
    }
    // Casting in UPPERCASE the received string char by char.
    while(string[0] != '\0'){
        string[0] = lowerupper == 'L' ? tolower(string[0]) : toupper(string[0]);
        string++;
    } 

}

// This function receive a message from the client or the server.
// There is no difference since the message format is the same.
// It takes as input the socket file descriptor from which read data.
// It returns a pointer to the struct Message allocated on the heap.
struct Message* receiveMessage(int fdfrom) {

    if (fdfrom < 0) {
        // Error
        printf("Error, the receiver of the message is invalid.\n");
    }

    int retvalue = 0;
    struct Message* readed = NULL;

    // Allocating heap memory to store the received message.
    readed = (struct Message*) malloc(sizeof(struct Message));
    if (readed == NULL) {
        // Error
        printf("Error in allocating heap memory for the received message.\n");
    }

    // Reading/Waiting for the message type.
    retvalue = read(fdfrom, &(readed->type), sizeof(readed->type));
    if (retvalue == -1) {
        // Error
        if (errno == EINTR) return readInterrupted(&readed);
        printf("Error in reading a message type.\n");
    }

    // Reading/Waiting for the message length.
    retvalue = read(fdfrom, &(readed->length), sizeof(readed->length));
    if (retvalue == -1) {
        // Error
        if (errno == EINTR) return readInterrupted(&readed);
        printf("Error in reading a message length.\n");
    }

    // Allocating heap memory to store the received message data.
    char* bufferstr = NULL;
    bufferstr = (char*) malloc(sizeof(char) * readed->length);
    if (bufferstr == NULL){
        // Error
        printf("Error in allocating heap memory for the message received data.\n");
    }
    readed->data = bufferstr;

    // Reading/Waiting for the message data.
    retvalue = read(fdfrom, readed->data, sizeof(char) * readed->length);
    if (retvalue == -1) {
        // Error
        if (errno == EINTR) return readInterrupted(&readed);
        printf("Error in reading a message data.\n");
    }

    return readed;

}

// This function will be executed when a read syscall fails due a signal of end game.
// EVERY COMPLETED REQUEST RECEIVED BEFORE THE TIMER WILL BE PROCESSED.
// In this case we we have been interrupted before the request has been fully transmitted
// by the client, and so it will be cancelled and not processed.
// This function delete the incomplete request.
void* readInterrupted(struct Message** readed) {

    struct Message* m = *readed;
    m->type = (char)0;
    m->length = 0;
    free(m->data);
    m->data = NULL;
    free(m);
    *readed = NULL;
    return NULL;

}

// This function send a message client -> server or server -> client.
// There is no difference since the message format is the same.
// It takes as input the file descriptor of the socket to wich the message will be send.
// Then it takes the struct Message fields, the type of the message and the data as char* (string).
// Note that the length of the length Message field will be automatically calculated based on data
// length with strlen().
void sendMessage(int fdto, char type, char* data) {

    if (fdto < 0) {
        // Error
        printf("Error, the recipient of the message is invalid.\n");
    }

    struct Message tosend;
    int retvalue;

    tosend.type = type;

    // Calculating and setting data length.
    tosend.length = 0U;
    if (data != NULL)
        tosend.length = sizeof(char) * strlen(data);

    tosend.data = data;
    if (data == NULL)
        tosend.data = 0;

    // Sending message type.
    retvalue = write(fdto, &(tosend.type), sizeof(tosend.type));
    if (retvalue == -1) {
        // Error
        printf("Error in sending message type.\n");
    }

    // Sending message length.
    retvalue = write(fdto, &(tosend.length), sizeof(tosend.length));
    if (retvalue == -1) {
        // Error
        printf("Error in sending message length.\n");
    }

    // Sending message data.
    retvalue = write(fdto, tosend.data, tosend.length);
    if (retvalue == -1) {
        // Error
        printf("Error in sending message data.\n");
    }

}

// This function destroy a Message sent o received by releasing allocated memory.
// It also modifies the value of the caller used pointer, setting it to NULL.
// This to avoid referring by mistake to memory released.
void destroyMessage(struct Message** m) {

    if (m == NULL || *m == NULL) {
        // Error
        printf("Error, cannot destroy an invalid message.\n");
    }

    // Releasing allocated memory and destroying the vars content.
    (*m)->type = (char)0;
    (*m)->length = 0u;
    free((*m)->data);
    (*m)->data = NULL;
    free((*m));
    *m = NULL;

}

// Wrapper lock/unlock mutexes to catch errors.
void mLock(pthread_mutex_t* m) {

    if (m == NULL) {
        // Error
        printf("Error, mLock() received an empty mutex.\n");
    }
    int retvalue;
    retvalue = pthread_mutex_lock(m);
    if (retvalue != 0) {
        // Error
        printf("Error in acquiring a mutex.\n");
    }

}

// Wrapper lock/unlock mutexes to catch errors.
void mULock(pthread_mutex_t* m) {

    if (m == NULL) {
        // Error
        printf("Error, mULock() received an empty mutex.\n");
    }

    int retvalue;
    retvalue = pthread_mutex_unlock(m);
    if (retvalue != 0) {
        // Error
        printf("Error in releasing a mutex.\n");
    }

}


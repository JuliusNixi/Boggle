#include "common.h"

// This function parse an IP and return the result code. If different by 1, there was an error.
// It also execute a inet_aton on a struct sockaddr_in*.
int parseIP(char* ip, struct sockaddr_in* sai) {

    if (ip == NULL || sai == NULL) {
        // Error
        printf("Error, parseIP() received a NULL ip or sockaddr_in*.\n");
    }

    // Lowering, LoCalhost and localhost and LOCALHOST, are now identical.
    toLowerOrUpperString(ip, 'L');

    // Converting localhost to 127.0.0.1.
    int retvalue;
    if (strcmp(ip, "localhost") == 0)
        retvalue = inet_aton("127.0.0.1", &sai->sin_addr);
    else
        retvalue = inet_aton(ip, &sai->sin_addr);
    return retvalue;

}

// This function normalize a string trasforming it in lower/upper case.
// 'L' means lowercase, 'U' upper that is is specified by lowerupper.
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
    // Casting the received string char by char.
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

// This function is called every time an error happens.
// The first int taken is 1 if we want print the errno value setted by some functions in case of error,
// 0 otherwise. The second int taken is 1 if we want kill the main thread, 0 otherwise. It is
// used for the errors that are critical for which continuing execution would be unsafe.
// The third int is used when an error is throw by the function printff(), a 
// custom wrapper of printf(). In this case handleError() could call printff() and printff()
// could call handleError() starting a mutual infinite recursion. To prevent this if the third
// int taken is 1 it means the error comes from printff() and so, the program exit without calling printff().
// The fourth int if setted to 1 if we want to kill the handleError() calling thread.
// The last parameter format and the elipsis are used to pass all the others args received to
// the printf().
void handleError(int printerrno, int killmain, int errorfromprintff, int killthisthread, const char* format,  ...) {

    // Wrong args or recursive error.
    if ((printerrno != 0 && printerrno != 1) || (killmain != 0 && killmain != 1)
    || (errorfromprintff != 0 && errorfromprintff != 1)
    || (killthisthread != 0 && killthisthread != 1)
    || errorfromprintff) {
        // Critical recursive error handler or wrong params.
        fprintf(stderr, "%s", format);
        fflush(stderr);
        // exit(EXIT_FAILURE); -> Recursive error if something happens in atExit()... use instead:
        _exit(1);
    }

    // Print errno.
    int retvalue;
    if (printerrno) {
        perror(format);
        retvalue = fflush(stderr);
        if (retvalue == EOF) {
            // Error
            // Critical recursive error.
            handleError(0, 1, 1, 1, RECURSIVE_MSG_ERR);
        }
    }else {
        // Normal print error (without errno).
        va_list args;
        va_start(args, format);
        printff(args, format);
        va_end(args);
    }
    
    // Main thread killer.
    if (killmain) {
        if (pthread_self() == mainthread) exit(EXIT_FAILURE); // atExit() called.
        retvalue = pthread_cancel(mainthread);
        // Critical recursive error.
        if (retvalue != 0) handleError(0, 1, 1, 1, RECURSIVE_MSG_ERR);
        else ; // Dead...
    } 

    if (killthisthread) pthread_exit(NULL);

}

// This function is a printf() custom wrapper.
// It takes as first arg a va_list struct.
// It's null if the printff() is normally called to print a message.
// If it's not null it means it has been called by the handleError() above function.
// In this last case the va_list struct is passed to vgprintf() to print on stderr.
// The const char* format and the elipsis (...) is used to forward all the other args to the printf().
void printff(va_list errorargs, const char* format, ...) {

    int retvalue;
    int retvalue2;
    // Called by handlerError().
    if (errorargs != NULL)  {
        retvalue = vfprintf(stderr, format, errorargs);
        retvalue2 = fflush(stderr);
    }else {
        // Called for a normal print (no error).
        va_list args;
        va_start(args, format);
        retvalue = vfprintf(stdout, format, args);
        retvalue2 = fflush(stdout);
        va_end(args);
    }

    if (retvalue < 0 || retvalue2 == EOF) {
         // Error
         // Critical recursive error.
        handleError(0, 1, 1, 1, RECURSIVE_MSG_ERR);
    }

}


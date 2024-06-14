#include "common.h"

// This function parse an IP and return the result code of inet_aton().
// If different by 1, there was an error.
// It also execute a inet_aton on a struct sockaddr_in* to initialize it.
int parseIP(char* ip, struct sockaddr_in* sai) {

    if (ip == NULL || sai == NULL) {
        // Error
        handleError(0, 1, 0, 1, "Error, parseIP() received a NULL IP or sockaddr_in*.\n");
    }

    // Lowering the string, LoCalhost and localhost and LOCALHOST, are now identical.
    toLowerOrUpperString(ip, 'L');

    // Converting localhost to 127.0.0.1.
    int retvalue;
    if (strcmp(ip, "localhost") == 0)
        retvalue = inet_aton("127.0.0.1", &sai->sin_addr);
    else
        retvalue = inet_aton(ip, &sai->sin_addr);
    return retvalue;

}

// This function normalize a string trasforming it in all lower/upper case.
// 'L' means lowercase, 'U' uppercase that is is specified by the input char lowerupper.
void toLowerOrUpperString(char* string, char lowerupper) {

    if (string == NULL) {
        // Error
        handleError(0, 1, 0, 1, "Error, toLowerOrUpperString() received an empty string.\n");
    }

    // Checking if lowerupper input is 'L' (means to change the string in lowercase),
    // or 'U' (means to change the string in UPPERCASE).
    lowerupper = toupper(lowerupper);
    if (lowerupper != 'L' && lowerupper != 'U') {
        // Error
        handleError(0, 0, 0, 0, "WARNING: Error in toLowerOrUpperString(), the second arg must be 'L' for lower or 'U' for upper. Trying to continue with UPPERCASE.\n");
    }

    // Casting the received string char by char.
    while(string[0] != '\0'){
        string[0] = lowerupper == 'L' ? tolower(string[0]) : toupper(string[0]);
        string++;
    } 

}

// This function receive a message from the client or the server.
// There is no difference since the message format is the  (struct Message).
// It takes as input the socket file descriptor from which read data.
// It returns a pointer to the new struct Message allocated on the heap with the readed data.
/*

If a read is interrupted by a signal...  
https://linux.die.net/man/3/read
https://stackoverflow.com/questions/66987029/can-a-read-or-write-system-call-continue-where-it-left-off-after-being-inter

Are read() and write() Thread-Safe?
https://en.wikipedia.org/wiki/Thread_safety
https://stackoverflow.com/questions/42442387/is-write-safe-to-be-called-from-multiple-threads-simultaneously
https://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html#tag_15_09_01
https://stackoverflow.com/questions/467938/stdout-thread-safe-in-c-on-linux
https://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html#tag_15_09_07
https://stackoverflow.com/questions/29331651/is-c-read-thread-safe
Not directly about the topic, but also clear:
https://stackoverflow.com/questions/56150004/thread-safety-vs-atomicity-in-c


##############################################################################
WARNING: The server's use of this function, ASSUMES that the client always sends a message ENTIRELY
(even if it is split into its fields) or NOT AT ALL.
Specifically:
    - If the client sends no message (i.e., no part of it) everything works of course.
    - If the server receive data that does not align with the data structure of the message,
      the behavior is undefined.
    - If the server receive LESS bytes than necessary (e.g., the client sends only the type
      field of the message struct), the server will wait INDEFINITELY, compromising the match
      for all users. Specifically, at the end of the match, the play/pause handler, i.e.,
      the thread executing signalsThread(), will send a SIGUSR1 signal to the thread stopped on read()
      that will abort the syscall, upon the handler's return, however, if nothing has been
      received the receiveMessage() function will return NULL and the control will pass normally
      to the calling clientHandler() function, but if, instead, something has been already written,
      the thread will expect all the rest of the message to arrive, waiting. At this point,
      the signalsThread() thread will wait for the clientHandler() thread to insert its result
      into the queue, but this may never happen if the client is uncooperative...
##############################################################################

*/
struct Message* receiveMessage(int fdfrom) {

    if (fdfrom < 0) {
        // Error
        printf("Error, the receiver of the message is invalid.\n");
    }

    int retvalue = 0;
    struct Message* readed = NULL;
    unsigned long toread;
    char* tmp;
    void* writingpointer;

    // Allocating heap memory to store the received message.
    readed = (struct Message*) malloc(sizeof(struct Message));
    if (readed == NULL) {
        // Error
        printf("Error in allocating heap memory for the received message.\n");
    }

    // Reading/Waiting for the message type.
    toread = sizeof(readed->type);
    writingpointer = &(readed->type);
readtype:
    retvalue = read(fdfrom, writingpointer, toread);
    // https://stackoverflow.com/questions/33053507/econnreset-in-send-linux-c
    if (retvalue == 0 || errno == ECONNRESET) {
        // Probably disconnection.
        free(readed);
        readed = NULL;
        return (void*)-1L;   
    }
    if (retvalue == -1) {
        // Error
        // Server:  destroying message, nothing has been received yet.
        //          Interrupted by a SIGNAL (end of game reached).
        if (errno == EINTR) {
            free(readed);
            readed = NULL;
            return readed;
        }
        // Another unmanageable error.
        printf("Error in reading a message type.\n");
    }

    // Server: Interrupted before of end reading (in the middle of a reading).
    //         Readed less bytes returned in retvalue.
    //         When i will return from the (end of game reached) SIGNAL management,
    //         I come back to read where i was left, reading only the unread data from the stream.
    if (retvalue < sizeof(readed->type)) {
        toread = sizeof(readed->type) - retvalue;
        // Server:  Incrementing the writingpointer to read to at offset position.
        //          Otherwise, i will read the new bytes, but i will overwrite the old ones
        //          (already readed previously).
        //          writingpointer += retvalue; --> CANNOT DO THIS due to a
        //          Warning by the compiler: arithmetic on a pointer to void is a GNU extension
        //          [-Wgnu-pointer-arith]
        //          To fix simply use char* and not void*.
        //          But since read() operates on bytes, are we sure that the lengths will match?
        //          Yes, sizeof(char) == 1 always, read here:
        //          https://stackoverflow.com/questions/3922958/void-arithmetic
        //          https://stackoverflow.com/questions/2215445/are-there-machines-where-sizeofchar-1-or-at-least-char-bit-8
        tmp = (char*) writingpointer;
        tmp += sizeof(char) * retvalue;
        writingpointer = (void*) tmp;
        goto readtype;
    }

    toread = sizeof(readed->length);
    writingpointer = &(readed->length);
readlength:
    // Reading/Waiting for the message length.
    retvalue = read(fdfrom, writingpointer, toread);
    if (retvalue == 0 || errno == ECONNRESET) {
        // Probably disconnection.
        free(readed);
        readed = NULL;
        return (void*)-1L;   
    }
    if (retvalue == -1) {
        // Error
        if (errno == EINTR) goto readlength;
        printf("Error in reading a message length.\n");
    }
    if (retvalue < sizeof(readed->length)) {
        toread = sizeof(readed->length) - retvalue;
        tmp = (char*) writingpointer;
        tmp += sizeof(char) * retvalue;
        writingpointer = (void*) tmp;
        goto readlength;
    }

    // Allocating heap memory to store the received message data.
    char* bufferstr = NULL;
    bufferstr = (char*) malloc(sizeof(char) * readed->length);
    if (bufferstr == NULL){
        // Error
        printf("Error in allocating heap memory for the message received data.\n");
    }
    readed->data = bufferstr;

    toread = sizeof(char) * readed->length;
    writingpointer = readed->data;
readdata:
    // Reading/Waiting for the message data.
    retvalue = read(fdfrom, writingpointer, toread);
    if (retvalue == 0 || errno == ECONNRESET) {
        // Probably disconnection.
        free(readed->data);
        free(readed);
        readed = NULL;
        return (void*)-1L;   
    }
    if (retvalue == -1) {
        // Error
        if (errno == EINTR) goto readdata;
        printf("Error in reading a message data.\n");
    }
    if (retvalue < sizeof(char) * readed->length) {
        toread = (sizeof(char) * readed->length) - retvalue;
        tmp = (char*) writingpointer;
        tmp += sizeof(char) * retvalue;
        writingpointer = (void*) tmp;
        goto readdata;
    }


    return readed;

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
        if (errno == EPIPE) {
            // Disconnection.
            printf("Cannot sendMessage(). Maybe a disconnection is happened.\n");
            return;
        }
        printf("Error in sending message type. Error: %d\n",retvalue);
    }

    // Sending message length.
    retvalue = write(fdto, &(tosend.length), sizeof(tosend.length));
    if (retvalue == -1) {
        // Error
        if (errno == EPIPE) {
            // Disconnection.
            printf("Cannot sendMessage(). Maybe a disconnection is happened.\n");
            return;
        }
        printf("Error in sending message length.\n");
    }

    // Sending message data.
    retvalue = write(fdto, tosend.data, tosend.length);
    if (retvalue == -1) {
        // Error
        if (errno == EPIPE) {
            // Disconnection.
            printf("Cannot sendMessage(). Maybe a disconnection is happened.\n");
            return;
        }
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
        fprintf(stderr, "%s", RECURSIVE_MSG_ERR);
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

     if (testmode) return;
    
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

// Thread safety of printf? No mixed output, but interleaving admitted.
// https://stackoverflow.com/questions/47912874/printf-function-on-multi-thread-program
// https://stackoverflow.com/questions/22366776/is-reading-and-writing-to-the-same-file-thread-safe
// Thread safety of fflush? Yes.
// https://stackoverflow.com/questions/39053670/can-a-program-call-fflush-on-the-same-file-concurrently
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

void makeKey(void){

    int retvalue;
    retvalue = pthread_key_create(&key, threadDestructor);
    if (retvalue != 0){
        // Error
        handleError(0, 1, 0, 1, "Error in pthread_key_create().\n");
    }

}

void threadSetup(void){
    void* ptr;

    int retvalue;
    retvalue = pthread_once(&key_once, makeKey);
    if (retvalue != 0){
        // Error
        handleError(0, 1, 0, 1, "Error in pthread_once().\n");
    }
    if ((ptr = pthread_getspecific(key)) == NULL) {
        ptr = malloc(sizeof(char));
        if (ptr == NULL) {
            // Error
            handleError(0, 1, 0, 1, "Error in malloc() in threadSetup().\n");
        }
        retvalue = pthread_setspecific(key, ptr);
        if (retvalue != 0){
            // Error
            handleError(0, 1, 0, 1, "Error in pthread_setspecific().\n");
        }
    }
    
}


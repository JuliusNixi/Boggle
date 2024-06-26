#include "common.h"

// This function parse an IP string and return the result code of inet_aton().
// If different by 1, there was an error.
// It also execute the inet_aton() on a struct sockaddr_in* to initialize it.
// It takes as input a char* (string) to the IP to parse, and the struct sockaddr_in*.
int parseIP(char* ip, struct sockaddr_in* sai) {

    if (ip == NULL || sai == NULL) {
        // Error
        handleError(0, 1, 0, 0, "Error, parseIP() received a NULL, IP or sockaddr_in*.\n");
    }

    // Lowering the string, LoCalhost and localhost and LOCALHOST, are now identical.
    toLowerOrUpperString(ip, 'L');

    // Converting localhost to 127.0.0.1 and executing inet_aton().
    int retvalue;
    if (strcmp(ip, "localhost") == 0)
        retvalue = inet_aton("127.0.0.1", &sai->sin_addr);
    else
        retvalue = inet_aton(ip, &sai->sin_addr);

    return retvalue;

}

// This function normalize a string trasforming it in all lower/upper case.
// 'L' means lowercase, 'U' uppercase that is specified by the input char lowerupper.
void toLowerOrUpperString(char* string, char lowerupper) {

    if (string == NULL) {
        // Error
        handleError(0, 0, 0, 0, "WARNING: Error in toLowerOrUpperString() received an empty string. Trying to continue.\n");
        return;
    }

    // Checking if lowerupper input is 'L' (means to change the string in lowercase),
    // or 'U' (means to change the string in UPPERCASE).
    lowerupper = toupper(lowerupper);
    if (lowerupper != 'L' && lowerupper != 'U') {
        // Error
        handleError(0, 0, 0, 0, "WARNING: Error in toLowerOrUpperString(), the second arg must be 'L' for lower or 'U' for upper. Trying to continue with UPPERCASE.\n");
        lowerupper = 'U';
    }

    // Casting the received string char by char.
    while(string[0] != '\0'){
        string[0] = lowerupper == 'L' ? tolower(string[0]) : toupper(string[0]);
        string++;
    } 

}

// This function receive a message from the client or the server.
// There is no difference since the message format is the same (struct Message).
// It takes as input the socket file descriptor from which read data.
// It returns a pointer to the new struct Message allocated on the heap with the readed data.
/*

If a read is interrupted by a signal...  
https://stackoverflow.com/questions/66987029/can-a-read-or-write-system-call-continue-where-it-left-off-after-being-inter

Are read() and write() Thread-Safe?
https://en.wikipedia.org/wiki/Thread_safety
https://stackoverflow.com/questions/42442387/is-write-safe-to-be-called-from-multiple-threads-simultaneously
https://stackoverflow.com/questions/467938/stdout-thread-safe-in-c-on-linux
https://stackoverflow.com/questions/29331651/is-c-read-thread-safe


#########################################################################################################
WARNING: The server's use of this function, ASSUMES that the client always sends a message ENTIRELY
(even if it is split into its fields) or NOT AT ALL.
Specifically:
    - If the client sends no message (i.e., no part of it) everything works of course.
    - If the server receive alla data, but that does not align with the data structure of
      the message, the behavior is undefined.
    - If the server receive LESS bytes than necessary (e.g., the client sends only the type
      field of the message struct), the server will wait INDEFINITELY, compromising the match
      for all users. Specifically, at the end of the match, the play/pause game
      handler, (signalsHandler()) will send a SIGUSR1 signal to the thread
      handling the user (clientHandler()) stopped on the read(), waiting for the client,
      that will abort the syscall, upon the signal handler's return, however, if nothing has been
      received the receiveMessage() function will return NULL and the control will pass normally
      to the calling clientHandler() function, but if, instead, something has been already written,
      the thread will expect all the rest of the message to arrive, waiting. At this point,
      the signalsThread() thread will wait for the clientHandler() thread to insert its result
      into the queue, but this may never happen if the client is uncooperative (not send
      the other missing part of the message)...
#########################################################################################################

*/
// ANCHOR receiveMessage()
struct Message* receiveMessage(int fdfrom) {

    if (fdfrom < 0) {
        // Error
        handleError(0, 0, 0, 1, "Error, the receiver of the message is invalid.\n");
    }

    int retvalue = 0;

    struct Message* readed = NULL;

    uli toread;
    void* writingpointer;
    char* tmp;

    // Allocating heap memory to store the received message.
    readed = (struct Message*) malloc(sizeof(struct Message));
    if (readed == NULL) {
        // Error
        handleError(0, 0, 0, 1, "Error in allocating heap memory for the received message.\n");
    }

    // Reading/Waiting for the message type.
    toread = sizeof(readed->type);
    writingpointer = &(readed->type);
readtype:
    retvalue = read(fdfrom, writingpointer, toread);
    // https://stackoverflow.com/questions/33053507/econnreset-in-send-linux-c
    if (retvalue == -1 && (errno == ECONNRESET || errno == EPIPE)) {
        // Probably a disconnection happened.
        free(readed);
        // sizeof(void*) = 8
        // sizeof(struct Message*) = 8
        // sizeof(long int) = 8
        // Trick to return a "special" value, since NULL is already used in another case.
        return (void*)-1L;   
    }
    if (retvalue == 0 && errno == ETIMEDOUT) {
        // Probably a disconnection happened.
        free(readed);
        return (void*)-1L;
    }
    if (retvalue == -1) {
        // Destroying message, nothing has been received yet.
        // Interrupted by a signal.
        if (errno == EINTR) {
            free(readed);
            readed = NULL;
            return readed;
        }
        // Error
        // Another unmanageable error.
        free(readed);
        handleError(1, 0, 0, 1, "Error in reading a message type.\n");
    }
    if (retvalue == 0) {
        // 0 bytes read.
        free(readed);
        readed = NULL;
        return readed;     
    }
    // Interrupted before of end reading (in the middle of a reading).
    // Readed less bytes returned in retvalue.
    // When i will return from the (end of game reached) SIGNAL management,
    // I come back to read where i was left, reading only the unread data from the stream.
    if (retvalue < sizeof(readed->type)) {
        toread = sizeof(readed->type) - retvalue;
        // Incrementing the writingpointer to read to at offset position.
        // Otherwise, i will read the new bytes, but i will overwrite the old ones
        // (already readed previously).
        // writingpointer += retvalue; --> CANNOT DO THIS due to a
        // Warning by the compiler: arithmetic on a pointer to void is a GNU extension
        // [-Wgnu-pointer-arith]
        // To fix simply use char* and not void*.
        // But since read() operates on bytes, are we sure that the lengths will match?
        // Yes, sizeof(char) == 1 always, read here:
        // https://stackoverflow.com/questions/3922958/void-arithmetic
        // https://stackoverflow.com/questions/2215445/are-there-machines-where-sizeofchar-1-or-at-least-char-bit-8
        tmp = (char*) writingpointer;
        tmp += (sizeof(char) * retvalue);
        writingpointer = (void*) tmp;
        goto readtype;
    }

    toread = sizeof(readed->length);
    writingpointer = &(readed->length);
readlength:
    // Reading/Waiting for the message length.
    retvalue = read(fdfrom, writingpointer, toread);
    if (retvalue == -1 && (errno == ECONNRESET || errno == EPIPE)) {
        // Probably a disconnection happened.
        free(readed);
        return (void*)-1L;   
    }
    if (retvalue == 0 && errno == ETIMEDOUT) {
        // Probably a disconnection happened.
        free(readed);
        return (void*)-1L;
    }
    if (retvalue == -1) {
        // Interrupted by a signal.
        if (errno == EINTR) goto readlength;
        // Error
        // Another unmanageable error.
        free(readed);
        handleError(1, 0, 0, 1, "Error in reading a message length.\n");
    }
    if (retvalue == 0) {
        // 0 bytes read.
        goto readlength;   
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
    if (readed->length == 0) readed->length = 1; // For null.
    bufferstr = (char*) malloc(sizeof(char) * readed->length);
    if (bufferstr == NULL) {
        // Error
        handleError(0, 0, 0, 0, "WARNING: Error in malloc() in receiveMessage(). Trying continuing and ignoring the message.\n");
        free(readed);
        return NULL;
    }
    readed->data = bufferstr;

    toread = sizeof(char) * readed->length;
    writingpointer = readed->data;
readdata:
    // Reading/Waiting for the message data.
    retvalue = read(fdfrom, writingpointer, toread);
    if (retvalue == -1 && (errno == ECONNRESET || errno == EPIPE)) {
        // Probably a disconnection happened.
        free(readed->data);
        free(readed);
        return (void*)-1L;   
    }
    if (retvalue == 0 && errno == ETIMEDOUT) {
        // Probably a disconnection happened.
        free(readed->data);
        free(readed);
        return (void*)-1L;
    }
    if (retvalue == -1) {
        // Interrupted by a signal.
        if (errno == EINTR) goto readdata;
        // Error
        // Another unmanageable error.
        handleError(1, 0, 0, 1, "Error in reading a message data.\n");
    }
    if (retvalue == 0) {
        // 0 bytes read.
        goto readdata;   
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

// ANCHOR sendMessage()
// This function send a message client -> server or server -> client.
// There is no difference since the message format is the same (struct Message).
// It takes as input the file descriptor of the socket to wich the message will be send.
// Then it takes the struct Message fields, the type of the message and the data as char* (string).
// Note that the length of the length Message field will be automatically calculated based on data
// length with strlen().
void* sendMessage(int fdto, char type, char* data) {

    if (fdto < 0) {
        // Error
        handleError(0, 0, 0, 1, "Error, the recipient of the message is invalid.\n");
    }

        switch (type){ 
            case MSG_MATRICE:
            case MSG_REGISTRA_UTENTE:
            case MSG_PAROLA:
            case MSG_ESCI :
            case MSG_ERR :
            case MSG_OK:
            case MSG_TEMPO_ATTESA:
            case MSG_TEMPO_PARTITA:
            case MSG_PUNTI_PAROLA:
            case MSG_PUNTI_FINALI:
            case MSG_IGNORATO: {
                // OK, nothing to do for all.
                ;
                break;
            }default:{
                // Error
                // Not recognized message.
                handleError(0, 1, 0, 0, "Error, called sendMessage() with an invalid message type.\n");
                break;
            } 

        } // Switch end.

    // Reminder, data can be NULL! We don't have to check it.

    int retvalue;

    struct Message tosend;
    uli towrite;
    void* writingpointer;
    char* tmp;

    tosend.type = type;

    // Calculating and setting data length.
    tosend.length = 1U;
    if (data != NULL)
        tosend.length = (unsigned) (sizeof(char) * (strlen(data) + 1)); // +1 for '\0'.

    char* s = NULL;
    if (data == NULL)
        tosend.data = 0;
    else{
        s = (char*) malloc(sizeof(char) * tosend.length);
        if (s == NULL) {
            // Error
            handleError(0, 0, 0, 0, "WARNING: Error in malloc() of data in sendMessage(). Nothing will be send.\n");
            return NULL;
        }
        strcpy(s, data);
        s[tosend.length] = '\0';
        tosend.data = s;
    }
    
    towrite = sizeof(tosend.type);
    writingpointer = &(tosend.type);
sendtype:
    // Writing the message type.
    retvalue = write(fdto, writingpointer, towrite);
    if (retvalue == -1 && (errno == ECONNRESET || errno == EPIPE)) {
        // Probably a disconnection happened.
        if(s) free(s);
        return (void*)-1L;   
    }
    if (retvalue == 0 && errno == ETIMEDOUT) {
        // Probably a disconnection happened.
        if (s) free(s);
        return (void*)-1L;
    }
    if (retvalue == -1) {
        // Interrupted by a signal.
        if (errno == EINTR) goto sendtype;
        // Error
        // Another unmanageable error.
        if (s) free(s);
        handleError(1, 0, 0, 1, "Error in writing a message type.\n");
    }
    if (retvalue == 0) {
        // 0 bytes read.
        goto sendtype;   
    }
    if (retvalue < sizeof(tosend.type)) {
        towrite = sizeof(tosend.type) - retvalue;
        tmp = (char*) writingpointer;
        tmp += (sizeof(char) * retvalue);
        writingpointer = (void*) tmp;
        goto sendtype;
    }


    towrite = sizeof(tosend.length);
    writingpointer = &(tosend.length);
sendlength:
    // Writing the message length.
    retvalue = write(fdto, writingpointer, towrite);
    if (retvalue == -1 && (errno == ECONNRESET || errno == EPIPE)) {
        // Probably a disconnection happened.
        if (s) free(s);
        return (void*)-1L;   
    }
    if (retvalue == 0 && errno == ETIMEDOUT) {
        // Probably a disconnection happened.
        if (s) free(s);
        return (void*)-1L;
    }
    if (retvalue == -1) {
        // Interrupted by a signal.
        if (errno == EINTR) goto sendlength;
        // Error
        // Another unmanageable error.
        if (s) free(s);
        handleError(1, 0, 0, 1, "Error in writing a message length.\n");
    }
    if (retvalue == 0) {
        // 0 bytes read.
        goto sendlength;   
    }
    if (retvalue < sizeof(tosend.length)) {
        towrite = sizeof(tosend.length) - retvalue;
        tmp = (char*) writingpointer;
        tmp += sizeof(char) * retvalue;
        writingpointer = (void*) tmp;
        goto sendlength;
    }

    towrite = tosend.length * sizeof(char);
    writingpointer = (tosend.data);
senddata: {
    // Writing the message data.
    char nulll = 0;
    if (writingpointer != 0) retvalue = write(fdto, writingpointer, towrite);
    else retvalue = write(fdto, &nulll, towrite);

    if (retvalue == -1 && (errno == ECONNRESET || errno == EPIPE)) {
        // Probably a disconnection happened.
        if (s) free(s);
        return (void*)-1L;   
    }

    if (retvalue == 0 && errno == ETIMEDOUT) {
        // Probably a disconnection happened.
        if (s) free(s);
        return (void*)-1L;
    }
    if (retvalue == -1) {
        // Interrupted by a signal.
        if (errno == EINTR) goto senddata;
        // Error
        // Another unmanageable error.
        if (s) free(s);
        handleError(1, 0, 0, 1, "Error in writing a message data.\n");
    }

    if (retvalue == 0) {
        // 0 bytes read.
        goto senddata;   
    }
    if (retvalue < tosend.length * sizeof(char)) {
        towrite = (sizeof(char) * tosend.length) - retvalue;
        tmp = (char*) writingpointer;
        tmp += sizeof(char) * retvalue;
        writingpointer = (void*) tmp;
        goto senddata;
    }
}

    if (s) free(s);
    return (void*)-2L;

}

// This function destroys a Message sent o received by releasing allocated memory.
// It also modifies the value of the caller used pointer, setting it to NULL.
// This to avoid referring by mistake to memory released.
void destroyMessage(struct Message** m) {

    if (m == NULL || *m == NULL) {
        // Error
        handleError(0, 0, 0, 0, "WARNING: Error, cannot destroy a NULL message. Trying ignore it.\n");
        return;
    }

    // Releasing allocated memory and destroying the vars content.
    (*m)->type = (char)0;
    (*m)->length = 0u;
    free((*m)->data);
    (*m)->data = NULL;
    free((*m));
    *m = NULL;

}

// Wrapper lock mutexes to catch errors.
void mLock(pthread_mutex_t* m) {

    if (m == NULL) {
        // Error
        handleError(0, 1, 0, 0, "Error, mLock() received an empty mutex.\n");
    }
    int retvalue = pthread_mutex_lock(m);
    if (retvalue != 0) {
        // Error
        handleError(0, 1, 0, 0, "Error in acquiring (locking) a mutex.\n");
    }

}

// Wrapper unlock mutexes to catch errors.
void mULock(pthread_mutex_t* m) {

    if (m == NULL) {
        // Error
        handleError(0, 1, 0, 0, "Error, mULock() received an empty mutex.\n");
    }

    int retvalue = pthread_mutex_unlock(m);
    if (retvalue != 0) {
        // Error
        handleError(0, 1, 0, 0, "Error in releasing (UNlocking) a mutex.\n");
    }

}

// This function is called every time an error happens.
// The first char taken is 1 if we want print the errno value setted by some functions 
// in case of error, 0 otherwise. 
// The second char taken is 1 if we want to kill the main thread, 0 otherwise.
// It is used for the errors that are critical for which continuing execution would be unsafe.
// The third char is used when an error is throw by the function printff(), a 
// custom wrapper of printf(), used also in this case to print error messages. 
// In this case handleError() could call printff() and printff()
// could call handleError(), starting a mutual infinite recursion. To prevent this if the third
// char taken is 1, it means the error comes from printff() and so,
// the program exit without calling again printff().
// The fourth char is setted to 1 if we want to kill the handleError() calling thread.
// Useful for example if we want to suppress a clientHandler() thread.
// The last parameter format and the elipsis are used to pass all the others args received to
// the printf().
void handleError(char printerrno, char killmain, char errorfromprintff, char killthisthread, const char* format,  ...) {

    // ANCHOR handleError()

    // Wrong args or recursive error.
    if ((printerrno != 0 && printerrno != 1)
    || (killmain != 0 && killmain != 1)
    || (errorfromprintff != 0 && errorfromprintff != 1)
    || (killthisthread != 0 && killthisthread != 1)
    || (killmain != 0 && killthisthread != 0) // No sense, both options simultaneously active. If i kill the main (by exiting, from the whole program), all threads will die, so obviously also this one.
    || errorfromprintff) {
        // Critical recursive error handler or wrong params.
        // This stops all the program without any cleanupper.
        fprintf(stderr, "%s", RECURSIVE_ERR_MSG);
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
            handleError(0, 0, 1, 0, RECURSIVE_ERR_MSG);
        }
    }else {
        // Normal print error (without errno).
        va_list args;
        va_start(args, format);
        printff(args, 0, format);
        va_end(args);
    }

    // To disable below options when used in test mode by Tests/tests.
     if (testmode) return;
    
    // Main thread killer.
    if (killmain) {
        // We are the main.
        if (pthread_self() == mainthread){
            pthread_cancel(mainthread);
            pthread_exit(NULL); // atExit() called after thread destructor.
        } 
        // We are not the main, we are another thread.
        // The pthread_cancel() allows the call to the killed thread destructor.
        retvalue = pthread_cancel(mainthread); // atExit() called after thread destructor.
        // Critical recursive error.
        if (retvalue != 0) handleError(0, 0, 1, 0, RECURSIVE_ERR_MSG);
        else return;
    } 

    // Current (this function caller) thread killer.
    if (killthisthread) pthread_exit(NULL);

}

// This function is a printf() custom wrapper.
// It takes as first arg a va_list struct.
// It's null if the printff() is normally called to print a message.
// If it's not null it means it has been called by the handleError() above function.
// In this last case the va_list struct is passed to vfprintf() to print on stderr.
// The const char* format and the elipsis (...) are used to forward all the
// other args to the printf().

// The second arg, is used to lock printing to all others threads. It's used when a thread
// needs to make a multi-line printing without allowing others threads to print something else
// in the middle.
// This because, as written below, the printf is thread-safe because it 
// prints the full string without intermixing others prints, but the prints order of the threads
// by default is undefinied and so must be handled.
// The second arg works with the global mutex defined mutexprint.
// If the second arg is 1, means that the caller (of this function) thread has locked
// this mutex, because the caller wants a safe multiline print, so this function simply prints.
// All the other non blocking prints will call this function by passing 0 as second arg.
// These latter prints will be blocked, because when the second arg is 0, before printing,
// this function will stuck on the mutexprint, waiting for the multiline prints (of another
// thread) to finish.
// After the multiline prints, the caller thread (of this function, by passing as second arg 1)
// must release the mutexprint!

// Thread safety of printf? No mixed output, but interleaving admitted.
// https://stackoverflow.com/questions/47912874/printf-function-on-multi-thread-program
// Thread safety of fflush? Yes.
// https://stackoverflow.com/questions/39053670/can-a-program-call-fflush-on-the-same-file-concurrently
void printff(va_list errorargs, char locking, const char* format, ...) {

    int retvalue;
    int retvalue2;

    if (locking != 0 && locking != 1) {
        // Error
        handleError(0, 0, 1, 0, RECURSIVE_ERR_MSG);
    }

    // Not use mLock() and mULock() beacuse could cause circular error.
    // They call handleError(), this calls printff(), this calls mLock()/mULock() and so on...
    if (locking == 0){
        retvalue = pthread_mutex_lock(&mutexprint);
        if (retvalue != 0) {
            // Error
            handleError(0, 0, 1, 0, RECURSIVE_ERR_MSG);
        }
    }

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
        handleError(0, 0, 1, 0, RECURSIVE_ERR_MSG);
    }

    if (locking == 0){
        retvalue = pthread_mutex_unlock(&mutexprint);
        if (retvalue != 0) {
            // Error
            handleError(0, 0, 1, 0, RECURSIVE_ERR_MSG);
        }
    }

}

// Executed only once (from the first calling thread) to create a once key.
// Used the thread key to register a thread destructor.
void makeKey(void){

    int retvalue;
    retvalue = pthread_key_create(&key, threadDestructor);
    if (retvalue != 0){
        // Error
        handleError(0, 1, 0, 0, "Error in pthread_key_create().\n");
    }
    printff(NULL, 0, "Once-key registered succesfully by the thread (ID): %lu.\n", (uli)pthread_self());

}

// This function MUST be executed by all threads to register the thread destructor.
// It sets a special (for each thread) data.
// But this feature is not used, it's used only because using this we can setup a thread
// destructor called as cleanupper during the threads termination.
void threadSetup(void){

    void* ptr = NULL;

    int retvalue;
    // Executed only once (from the first calling thread).
    retvalue = pthread_once(&key_once, makeKey);
    if (retvalue != 0){
        // Error
        handleError(0, 1, 0, 0, "Error in pthread_once().\n");
    }


    // Setting LOCAL thread's data.
    // Never used, but for clarity.
    char data;
    pthread_t current = pthread_self();
    if (current == mainthread) data = 'M'; // Main thread.
    else data = 'O'; // Other thread.

    // This if is executed only the first time (by each thread) to setup the data.
    // WARNING, IF THE DATA IS NOT SETUP, THE DESTRUCTOR WON'T BE EXECUTED!
    if ((ptr = pthread_getspecific(key)) == NULL) {
        ptr = malloc(sizeof(char));
        if (ptr == NULL) {
            // Error
            handleError(0, 0, 0, 1, "Error in malloc() in threadSetup().\n");
        }
        *((char*)(ptr)) = data;
        // ... Other things that may be necessary carried out ONLY the FIRST TIME FOR EACH THREAD. ...
        retvalue = pthread_setspecific(key, ptr);
        if (retvalue != 0){
            // Error
            handleError(0, 0, 0, 1, "Error in pthread_setspecific().\n");
        }
        printff(NULL, 0, "Specific key setted for thread (ID): %lu.\n", (uli)pthread_self());
    }

    // Retrieve thread specific data (not necessary in this usage).
    // char* dataptr = pthread_getspecific(key)
    ;

    return;
    
}

// ANCHOR bannerCreator()
// This function is for purely aesthetic.
// It creates a very very simple string used to divide the sections in the program's output,
// to increase the clarity of reading.
// It returns an heap allocated new string.
// It takes as input the total string length desired.
// It takes as input the bannertext, that is a string that will be placed in the center
// of the divider.
// It takes as input the nspaces that rapresent how many spaces should be between the bannersymbol
// and the bannertext.
// It takes as input the bannersymbol that rapresent the symbol that will costitute the string.
// It takes as input the voidstringornot, this will be 1 if the string should be only composed
// by bannersymbol, 0 otherwise and will match the structure previously described.
char* bannerCreator(uli totalstrlength, uli nspaces, char* bannertext, char bannersymbol, char voidstringornot){
    
    if (nspaces > totalstrlength || (voidstringornot != 0 && voidstringornot != 1) || (voidstringornot == 1 && bannertext != NULL)){
        // Error
        handleError(0, 0, 0, 0, "WARNING: Error in the bannerCreator(), invalid input/s. Trying to continue.\n");
        return NULL;
    }
    
    uli bannertextlength = voidstringornot ? 2U : strlen(bannertext);

    // Calculate X, which is the number of times the symbol will be repeated.
    uli X = (totalstrlength - nspaces * 2LU - bannertextlength) / 2LU;
    
    // Calculate the total size needed for the new string.
    uli totalsize = X * 2LU + nspaces * 2LU + bannertextlength + 1LU;
    
    // Allocate memory on the heap for the new string.
    char* result = (char*) malloc(totalsize * sizeof(char));
    if (result == NULL) {
        // Error
        handleError(0, 0, 0, 0, "WARNING: Error, in malloc() in the bannerCreator(), trying to continue.\n");
        return result;
    }
    
    // Fill the string with the symbol, spaces, and the input string.
    uli index = 0LU;
    for (uli i = 0LU; i < X; i++) {
        result[index++] = bannersymbol;
    }
    for (uli i = 0LU; i < nspaces; i++) {
        result[index++] = ' ';
    }
    for (uli i = 0LU; i < bannertextlength; i++) {
        if (voidstringornot == 0) result[index++] = bannertext[i];
        else result[index++] = '\n'; // Will be substitute later.
    }
    for (uli i = 0LU; i < nspaces; i++) {
        result[index++] = ' ';
    }
    for (uli i = 0LU; i < X; i++) {
        result[index++] = bannersymbol;
    }
    
    // Null-terminate the string.
    result[index] = '\0';

    if (voidstringornot) {
        char* s = result;
        while (1) {
            if (s[0] == '\0') break;
            if (s[0] != bannersymbol) s[0] = bannersymbol;
            s++;
        }
    }
    
    return result;

}

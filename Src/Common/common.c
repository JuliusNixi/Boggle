#include "common.h"

// ANCHOR toLowerOrUpperString()
// This function normalizes a string received as input, trasforming it in all lower/UPPER case.
// 'L' means lowercase, 'U' UPPERCASE, that is specified by the input char lowerupper.
void toLowerOrUpperString(char* string, char lowerupper) {

    if (string == NULL) {
        // Error
    }

    // Checking if lowerupper input is 'L' (means to change the string in lowercase),
    // or 'U' (means to change the string in UPPERCASE).
    lowerupper = toupper(lowerupper);
    if (lowerupper != 'L' && lowerupper != 'U') {
        // Error
    }

    // Casting the received string char by char.
    while(string[0] != '\0'){
        string[0] = lowerupper == 'L' ? tolower(string[0]) : toupper(string[0]);
        string++;
    } 

}

// ANCHOR parseIP()
// This function parse an IP string, received as input with char* ip,
// by using the inet_aton() function, and returns the result code of inet_aton().
// If different by 1, there was an error.
// It executes the inet_aton() on a struct sockaddr_in* sai received by input to initialize it.
// It takes as input a char* (string) to the IP to parse, and the struct sockaddr_in* to fill.
int parseIP(char* ip, struct sockaddr_in* sai) {

    if (ip == NULL || sai == NULL) {
        // Error
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

// ANCHOR receiveMessage()
// This function receive a message from the client or the server.
// There is no difference since the message format is the same (struct Message).
// It takes as input the socket file descriptor from which read data.
// It takes as input also the char* resultcode, a pointer to a char where will be write the
// operation's result code.
// This code could be:
// - 0: A disconnection happened.
// - 1: Nornally interrupted by a signal.
// - 2: Unexpected error.
// - 3: Read 0 bytes (at the message beginning, so the message's type).
// - 4: Completed message received succesfully.
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
WARNING: The server's use of this function, ASSUMES that the client will always send a message ENTIRELY
(even if it is split into its fields) or NOT AT ALL.
Specifically:
    - If the client sends no message (i.e., no part of it) everything works of course.
    - If the server receive some data, but that does not align with the data structure of
      the message, the behavior is undefined.
    - If the server receive LESS bytes than necessary (e.g., the client sends only the type
      field of the Message struct), the server will wait INDEFINITELY for the rest of the message, 
      compromising the match for all users. Specifically, at the end of the match, the play/pause game
      handler, (signalsHandler()) will send a SIGUSR1 signal to the thread
      handling the user (clientHandler()) stopped on the read(), waiting for the client,
      that will abort the syscall, upon the signal handler's return, however, if nothing has been
      received the receiveMessage() function will return NULL and the control will pass normally
      to the calling clientHandler() function, but if, instead, something has been already written,
      the thread will expect all the rest of the message to arrive, waiting. At this point,
      the signalsThread() thread will wait for the clientHandler() thread to insert its result
      into the queue, but this may never happen if the client is uncooperative (not send
      the other missing part of the message).
#########################################################################################################

*/
struct Message* receiveMessage(int fdfrom, char* resultcode) {

    if (fdfrom < 0) {
        // Error
    }

    int retvalue;

    struct Message* readed = NULL;

    uli toread;
    void* writingpointer;
    char* tmp;

    // Allocating heap memory to store the incoming received message.
    readed = (struct Message*) malloc(sizeof(struct Message));
    if (readed == NULL) {
        // Error
    }

    // Reading/Waiting for the message type.
    toread = sizeof(readed->type);
    writingpointer = &(readed->type);
readtype:
    retvalue = read(fdfrom, writingpointer, toread);
    // https://stackoverflow.com/questions/33053507/econnreset-in-send-linux-c
    // EPIPE is obtained when writing to a closed target.
    // ECONNRESET is obtained when writing to a closed target and its buffer (of the target)
    // is not empty.
    if (retvalue == -1 && (errno == ECONNRESET || errno == EPIPE)) {
        // Probably a disconnection happened.
        destroyMessage(&readed);
        *resultcode = 0;
        return NULL;   
    }
    if (retvalue == 0 && errno == ETIMEDOUT) {
        // Probably a disconnection happened.
        destroyMessage(&readed);
        *resultcode = 0;
        return NULL;
    }
    if (retvalue == -1) {
        // Interrupted by a signal, normal, we are in end game.
        if (errno == EINTR) {
            destroyMessage(&readed);
            *resultcode = 1;
            return NULL;
        }
        // Error
        // Another unmanageable error.
        destroyMessage(&readed);
        *resultcode = 2;
        return NULL;
    }
    if (retvalue == 0) {
        // 0 bytes read.
        destroyMessage(&readed);
        *resultcode = 3;
        return NULL;     
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
        // warning by the compiler: arithmetic on a pointer to void is a GNU extension
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
        destroyMessage(&readed);
        *resultcode = 0;
        return NULL;   
    }
    if (retvalue == 0 && errno == ETIMEDOUT) {
        // Probably a disconnection happened.
        destroyMessage(&readed);
        *resultcode = 0;
        return NULL;
    }
    if (retvalue == -1) {
        // Interrupted by a signal, normal, we are in end game.
        if (errno == EINTR) goto readlength;
        // Error
        // Another unmanageable error.
        destroyMessage(&readed);
        *resultcode = 2;
        return NULL;
    }
    if (retvalue == 0) {
        // 0 bytes read.
        goto readlength;   
    }
    if (retvalue < sizeof(readed->length)) {
        toread = sizeof(readed->length) - retvalue;
        tmp = (char*) writingpointer;
        tmp += (sizeof(char) * retvalue);
        writingpointer = (void*) tmp;
        goto readlength;
    }

    // Allocating heap memory to store the received message data.
    char* bufferstr = NULL;
    if (readed->length != 0U) {
        // Read data NOT NULL.
        bufferstr = (char*) malloc(sizeof(char) * readed->length);
        if (bufferstr == NULL) {
            // Error
        }
        readed->data = bufferstr;
        toread = sizeof(char) * readed->length;
        writingpointer = readed->data;
readdata:
        // Reading/Waiting for the message data.
        retvalue = read(fdfrom, writingpointer, toread);
        if (retvalue == -1 && (errno == ECONNRESET || errno == EPIPE)) {
            // Probably a disconnection happened.
            destroyMessage(&readed);
            *resultcode = 0;
            return NULL;  
        }
        if (retvalue == 0 && errno == ETIMEDOUT) {
            // Probably a disconnection happened.
            destroyMessage(&readed);
            *resultcode = 0;
            return NULL;  
        }
        if (retvalue == -1) {
            // Interrupted by a signal, normal, we are in end game.
            if (errno == EINTR) goto readdata;
            // Error
            // Another unmanageable error.
            destroyMessage(&readed);
            *resultcode = 2;
            return NULL;
        }
        if (retvalue == 0) {
            // 0 bytes read.
            goto readdata;   
        }
        if (retvalue < (sizeof(char) * readed->length)) {
            toread = (sizeof(char) * readed->length) - retvalue;
            tmp = (char*) writingpointer;
            tmp += (sizeof(char) * retvalue);
            writingpointer = (void*) tmp;
            goto readdata;
        }
    }else{
        // Read data IS NULL.
        toread = sizeof(char*);
        writingpointer = &bufferstr;
readdatanull:
        retvalue = read(fdfrom, writingpointer, toread);     
        if (retvalue == -1 && (errno == ECONNRESET || errno == EPIPE)) {
            // Probably a disconnection happened.
            destroyMessage(&readed);
            *resultcode = 0;
            return NULL;  
        }
        if (retvalue == 0 && errno == ETIMEDOUT) {
            // Probably a disconnection happened.
            destroyMessage(&readed);
            *resultcode = 0;
            return NULL;  
        }
        if (retvalue == -1) {
            // Interrupted by a signal, normal, we are in end game.
            if (errno == EINTR) goto readdatanull;
            // Error
            // Another unmanageable error.
            destroyMessage(&readed);
            *resultcode = 2;
            return NULL;
        }
        if (retvalue == 0) {
            // 0 bytes read.
            goto readdatanull;   
        }
        if (retvalue < sizeof(char*)) {
            toread = (sizeof(char*)) - retvalue;
            tmp = (char*) writingpointer;
            tmp += (sizeof(char*) * retvalue);
            writingpointer = (void*) tmp;
            goto readdatanull;
        }              
    }

    *resultcode = 4;
    return readed;

}

// ANCHOR sendMessage()
// This function send a message from the client or the server.
// There is no difference since the message format is the same (struct Message).
// It takes as input the file descriptor of the socket to wich the message will be sent.
// Then it takes the struct Message fields: the type of the message and the data as char* (string).
// Note that the length of the length Message field will be automatically calculated based on data
// length with strlen(), or will be 0 if data is passed NULL.
// It returns a char containing the code result.
// This code could be:
// - 0: A disconnection happened.
// - 2: Unexpected error.
// - 1: Completed message sent succesfully.
char sendMessage(int fdto, char type, char* data) {

    if (fdto < 0) {
        // Error
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
                break;
            } 

        }

    // Reminder, data can be NULL! We have to check it.

    int retvalue;

    struct Message tosend;
    uli towrite;
    void* writingpointer;
    char* tmp;

    tosend.type = type;

    // Calculating data length.
    tosend.length = 0U;
    if (data != NULL)
        tosend.length = (unsigned) (sizeof(char) * (strlen(data) + 1)); // +1 for '\0'.

    // Preparing data content.
    char* s;
    if (data == NULL)
        s = 0;
    else{
        s = (char*) malloc(sizeof(char) * tosend.length);
        if (s == NULL) {
            // Error
        }
        strcpy(s, data);
        s[strlen(data)] = '\0';
    }

    tosend.data = s;
    
    towrite = sizeof(tosend.type);
    writingpointer = &(tosend.type);
sendtype:
    // Writing the message type.
    retvalue = write(fdto, writingpointer, towrite);
    if (retvalue == -1 && (errno == ECONNRESET || errno == EPIPE)) {
        // Probably a disconnection happened.
        if (s) free(s);
        return 0;    
    }
    if (retvalue == 0 && errno == ETIMEDOUT) {
        // Probably a disconnection happened.
        if (s) free(s);
        return 0; 
    }
    if (retvalue == -1) {
        // Interrupted by a signal, normal, we are in end game.
        if (errno == EINTR) goto sendtype;
        // Error
        // Another unmanageable error.
        if (s) free(s);
        return 2;
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
        return 0; 
    }
    if (retvalue == 0 && errno == ETIMEDOUT) {
        // Probably a disconnection happened.
        if (s) free(s);
        return 0; 
    }
    if (retvalue == -1) {
        // Interrupted by a signal, normal, we are in end game.
        if (errno == EINTR) goto sendlength;
        // Error
        // Another unmanageable error.
        if (s) free(s);
        return 2;
    }
    if (retvalue == 0) {
        // 0 bytes read.
        goto sendlength;   
    }
    if (retvalue < sizeof(tosend.length)) {
        towrite = sizeof(tosend.length) - retvalue;
        tmp = (char*) writingpointer;
        tmp += (sizeof(char) * retvalue);
        writingpointer = (void*) tmp;
        goto sendlength;
    }

    // Writing the message data.
    if (s) {
        // Message's data NOT NULL.
        towrite = tosend.length * sizeof(char);
        writingpointer = tosend.data;
senddata: {
        retvalue = write(fdto, writingpointer, towrite);
        if (retvalue == -1 && (errno == ECONNRESET || errno == EPIPE)) {
            // Probably a disconnection happened.
            if (s) free(s);
            return 0;   
        }
        if (retvalue == 0 && errno == ETIMEDOUT) {
            // Probably a disconnection happened.
            if (s) free(s);
            return 0;  
        }
        if (retvalue == -1) {
            // Interrupted by a signal, normal, we are in end game.
            if (errno == EINTR) goto senddata;
            // Error
            // Another unmanageable error.
            if (s) free(s);
            return 2;
        }
        if (retvalue == 0) {
            // 0 bytes read.
            goto senddata;   
        }
        if (retvalue < tosend.length * sizeof(char)) {
            towrite = (sizeof(char) * tosend.length) - retvalue;
            tmp = (char*) writingpointer;
            tmp += (sizeof(char) * retvalue);
            writingpointer = (void*) tmp;
            goto senddata;
        }
}
    }else{
        // Message's data IS NULL.
        towrite = sizeof(char*);
        char* nulll = 0;
        writingpointer = &nulll;
senddatanull: {
        retvalue = write(fdto, writingpointer, towrite);
        if (retvalue == -1 && (errno == ECONNRESET || errno == EPIPE)) {
            // Probably a disconnection happened.
            if (s) free(s);
            return 0;   
        }
        if (retvalue == 0 && errno == ETIMEDOUT) {
            // Probably a disconnection happened.
            if (s) free(s);
            return 0;  
        }
        if (retvalue == -1) {
            // Interrupted by a signal, normal, we are in end game.
            if (errno == EINTR) goto senddatanull;
            // Error
            // Another unmanageable error.
            if (s) free(s);
            return 2;
        }
        if (retvalue == 0) {
            // 0 bytes read.
            goto senddatanull;   
        }
        if (retvalue < sizeof(char*)) {
            towrite = (sizeof(char*)) - retvalue;
            tmp = (char*) writingpointer;
            tmp += (sizeof(char) * retvalue);
            writingpointer = (void*) tmp;
            goto senddata;
        }
}       
    }

    // Message sent.
    if (s) free(s);
    return 1;

}

// ANCHOR destroyMessage()
// This function destroys a Message by releasing allocated heap memory.
// It also modifies the value of the caller used pointer, setting it to NULL.
// This to avoid referring by mistake to memory already released.
void destroyMessage(struct Message** m) {

    if (m == NULL || *m == NULL) {
        // Error
    }

    // Releasing allocated memory and destroying the vars content.
    (*m)->type = (char)0;
    (*m)->length = 0U;
    if ((*m)->data != NULL) free((*m)->data);
    (*m)->data = NULL;
    free((*m));
    *m = NULL;

}


// Thread safety of printf? No mixed output, but interleaving admitted.
// https://stackoverflow.com/questions/47912874/printf-function-on-multi-thread-program
// Thread safety of fflush? Yes.
// https://stackoverflow.com/questions/39053670/can-a-program-call-fflush-on-the-same-file-concurrently


// ANCHOR bannerCreator()
// This function is for purely aesthetic.
// It creates a very simple string used to divide the sections in the program's output (both client
// and server) to increase the clarity of reading.
// It returns an heap allocated new string containing the banner.
// It takes as input the totalstrlength, so the total string length desired.
// It takes as input the bannertext, that is a string that will be placed in the center
// of the divider.
// It takes as input the nspaces that rapresent how many spaces should be between the bannersymbol
// and the bannertext.
// It takes as input the bannersymbol that rapresent the symbol that will costitute the string.
// It takes as input the voidstringornot, this will be 1 if the string should be only composed
// by bannersymbol, 0 otherwise (so will contain bannertext) and will match the structure
// previously described.
char* bannerCreator(uli totalstrlength, uli nspaces, char* bannertext, char bannersymbol, char voidstringornot){
    
    // Inputs check.
    if (totalstrlength == 0LU ||
     nspaces > totalstrlength || 
     (voidstringornot != 0 && voidstringornot != 1) ||
     (voidstringornot == 1 && bannertext != NULL)){
        // Error
    }
    
    // When voidstringornot is 1, to not have to differentiate the code too unnecessarily,
    // I will use an empty string as bannertext and i will replace its characters
    // (of the empty string) at the end with bannersymbol.
    char emptystring[] = "EMPTY";
    if (voidstringornot) bannertext = emptystring;

    // Searching for bannersymbol in bannertext.
    char* s = bannertext;
    while(1) {
        if (s[0] == '\0') break;
        if (s[0] == bannersymbol) {
            // Error
            // bannertext cannot contains bannersymbol.
        }
        s++;
    }

    uli bannertextlength = strlen(bannertext);

    // Calculate X, which is the number of times the symbol will be repeated.
    uli X = (totalstrlength - nspaces * 2LU - bannertextlength) / 2LU;
    
    // Calculate the total size needed for the new string.
    uli totalsize = X * 2LU + nspaces * 2LU + bannertextlength + 1LU;
    
    // Allocate memory on the heap for the new string.
    char* result = (char*) malloc(totalsize * sizeof(char));
    if (result == NULL) {
        // Error
    }
    
    // Fill the string with the symbol, spaces, and the input string.
    uli index = 0LU;
    for (uli i = 0LU; i < X; i++)
        result[index++] = bannersymbol;
    
    for (uli i = 0LU; i < nspaces; i++)
        result[index++] = ' ';
    
    for (uli i = 0LU; i < bannertextlength; i++)
        result[index++] = bannertext[i];
    
    for (uli i = 0LU; i < nspaces; i++) 
        result[index++] = ' ';
    
    for (uli i = 0LU; i < X; i++) 
        result[index++] = bannersymbol;
    
    
    // Null-terminate the string.
    result[index] = '\0';

    if (voidstringornot) {
        s = result;
        while (1) {
            if (s[0] == '\0') break;
            if (s[0] != bannersymbol) s[0] = bannersymbol;
            s++;
        }
    }
    
    return result;

}


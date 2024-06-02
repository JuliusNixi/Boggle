#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <stdarg.h>

// Messages types.
#define MSG_OK 'K'
#define MSG_ERR 'E'
#define MSG_REGISTRA_UTENTE 'R'
#define MSG_MATRICE 'M'
#define MSG_TEMPO_PARTITA 'T' // Time to end game.
#define MSG_TEMPO_ATTESA 'A' // Time remaining to the start of a new game, pause left time.
#define MSG_PAROLA 'W'
#define MSG_PUNTI_FINALI 'F'
#define MSG_PUNTI_PAROLA 'P'

#define MSG_ESCI 'Q' // Message sent by the client to the server or vice versa to close the connection.
#define MSG_IGNORATO 'I' // Special message that the server send to the client to advise it of a
// received request not computed due to the end of game.

#define BUFFER_SIZE 1024  // Size of the buffers that will be used in some cases.

#define RECURSIVE_MSG_ERR  "Critical recursive error detected, exiting...\n" // Message to print when an error happens during the management of a previous one, potentially leading to endless mutual recursion.

// Struct of the message that will be used in the communication between server and clients.
struct Message {
    char type;  // Type of message as above.
    unsigned int length; // Length of the below data field.
    char* data;  // Message content, heap allocated string, null terminated.
};

struct sockaddr_in server_addr; // Socket server address.

sigset_t signal_mask; // Signal mask to handle signals (SIGINT and SIGALRM), blocking it, 
// to just let them handle to the thread below.
pthread_t sig_thr_id; // Thread that will handle the signals (SIGINT and SIGALRM).

pthread_t mainthread; // Main thread.

pthread_key_t key;  // Used to set custom threads destructors.
pthread_once_t key_once; // Used to set custom threads destructors.

// Functions used both in client and server, their implementation normally is
// the same, is done in common.c.
int parseIP(char*, struct sockaddr_in*);
void toLowerOrUpperString(char*, char);

// Present both in client and server, but with DIFFERENT IMPLEMENTATION.
void atExit(void);
void* signalsThread(void*);
// Thread safety of printf? No mixed output, but interleaving admitted.
// https://stackoverflow.com/questions/47912874/printf-function-on-multi-thread-program
// Thread safety of fflush? Yes.
// https://stackoverflow.com/questions/39053670/can-a-program-call-fflush-on-the-same-file-concurrently
void threadDestructor(void*);
void signalsThreadDestructor(void*);
void makeKey(void);
void threadSetup(void);
////////////////////////////////////////////////////////////////////////

struct Message* receiveMessage(int);
void sendMessage(int, char, char*);
void destroyMessage(struct Message**);
void mLock(pthread_mutex_t*);
void mULock(pthread_mutex_t*);
void* readInterrupted(struct Message**); // Used by receiveMessage().
void handleError(int, int, int, int, const char*, ...);
void printff(va_list, const char*, ...);

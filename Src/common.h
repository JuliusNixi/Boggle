#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
// from man
// errno is thread-local; setting it in one thread does not affect its value in any other thread.
#include <errno.h>
#include <stdarg.h>
#include <signal.h>

// Messages types as described in the project text.
#define MSG_OK 'K'
#define MSG_ERR 'E'
#define MSG_REGISTRA_UTENTE 'R' // Register a user.
#define MSG_MATRICE 'M'  // Get current game matrix.
#define MSG_TEMPO_PARTITA 'T' // Time to end game.
#define MSG_TEMPO_ATTESA 'A' // Time remaining to the start of a new game, pause left time.
#define MSG_PAROLA 'W' // Submit a word.
#define MSG_PUNTI_FINALI 'F' // End game scoreboard.
#define MSG_PUNTI_PAROLA 'P'  // Word guessed.

// Added by me.
#define MSG_ESCI 'Q' // Message sent by the client to the server or vice versa to close the connection.
#define MSG_IGNORATO 'I' // Message sent by the server to the client to notify that the sent request received will be ignored since it was RECEIVED AFTER the timer of end game trigger.

#define BUFFER_SIZE 1024  // Size of the buffers that will be used in some cases.

#define RECURSIVE_ERR_MSG  "Critical recursive error detected, exiting...\n" // Message to print when an error happens during the management of a previous one, potentially leading to endless mutual recursion.

typedef unsigned long int uli; // Shortcut used sometimes.

struct Message { // Struct of the message that will be used in the communication between server and clients.
    char type;  // Type of message as above.
    unsigned int length; // Length of the below data field.
    char* data;  // Message content, heap allocated string, null terminated.
};

struct sockaddr_in server_addr; // Socket server address.

sigset_t signal_mask; // Signal mask to handle signals (SIGINT and SIGALRM), blocking it, to just let them handle to the thread below. SIGPIPE blocked and handled but not from external functions.
pthread_t sig_thr_id; // Thread that will handle the signals (SIGINT and SIGALRM).

pthread_t mainthread; // Main thread.

pthread_key_t key;  // Used to set custom threads destructors.
pthread_once_t key_once; // Used to set custom threads destructors.

char testmode;  // Used in Tests/tests.c to disable error handling.

// Present both in client and server, but with DIFFERENT IMPLEMENTATION.
void* signalsThread(void*);
void atExit(void);
void threadDestructor(void*);

// Functions used both in client and server, their implementation normally is
// the same, is done in common.c.
// More infos in common.c.
int parseIP(char*, struct sockaddr_in*);
void toLowerOrUpperString(char*, char);
struct Message* receiveMessage(int);
void* sendMessage(int, char, char*);
void destroyMessage(struct Message**);
void mLock(pthread_mutex_t*);
void mULock(pthread_mutex_t*);
void handleError(char, char, char, char, const char*, ...);
void printff(va_list, const char*, ...);
void makeKey(void);
void threadSetup(void);


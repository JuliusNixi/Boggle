#include <ctype.h>
#include <stdio.h>
#include <signal.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
// From man:
// errno is thread-local; setting it in one thread does not affect its value in any other thread.
#include <errno.h>
#include <string.h>
// _GNU_SOURCE needed to use pthread_setname_np() on Linux.
#define _GNU_SOURCE
#include <pthread.h>
#if defined(__APPLE__)
    // On macOS pthread_setname_np() implementation is "void pthread_setname_np(const char *name);"
    #define pthread_setname_np(THREAD_ID, THREAD_NAME) pthread_setname_np(THREAD_NAME)
#elif defined(__linux__)
    // On Linux pthread_setname_np() implementation is "int pthread_setname_np(pthread_t thread, const char *name);"
    #define pthread_setname_np(THREAD_ID, THREAD_NAME) pthread_setname_np(THREAD_ID, THREAD_NAME)
#endif
// I will write in all the code the Linux's implementation, and with the above code, if compiling
// on macOS the second arg will be discarded without throwing errors.
// Banner settings used in client and server banner.
// pthread_setname_np() is used to easily understand the various thread types during debugging.
#define BANNER_LENGTH 80LU
#define BANNER_SYMBOL '#'
#define BANNER_NSPACES 4LU

pthread_t mainthread; // Main thread.

sigset_t signalmask; // Signal mask to handle signals, blocking it, to just let them handle to a dedicated thread.
pthread_t signalsthread; // Thread that will handle the signals.

struct sockaddr_in server_addr; // Socket server address.

char setupfinished; // Used to notify the main thread to continue after initialization of all others threads.
pthread_mutex_t setupmutex; // Used to synchronize read/write threads operations with the above var.

#define BUFFER_SIZE 1024U  // Size of the buffers that will be used in some cases.

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

struct Message { // Struct of the message that will be used in the communication between server and clients.
    char type;  // Type of message as above.
    unsigned int length; // Length of the below data field, 0 if the below field is NULL.
    char* data;  // Message content, heap allocated string, null terminated if present, otherwise NULL.
};

typedef unsigned long int uli; // Shortcut used often.

pthread_mutex_t printmutex; // Used to print blocks of lines knowing that will not be others prints interleaved from other threads.

// Present both in client and server, but with DIFFERENT IMPLEMENTATION.
void* signalsThread(void*);

// Functions used both in client and server, their implementation normally is
// the same, is done in common.c.
// More infos in common.c.
int parseIP(char*, struct sockaddr_in*);
void toLowerOrUpperString(char*, char);
struct Message* receiveMessage(int, char*);
char sendMessage(int, char, char*);
void destroyMessage(struct Message**);
char* bannerCreator(uli, uli, char*, char, char);


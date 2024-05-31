#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>

#define MSG_OK 'K'
#define MSG_ERR 'E'
#define MSG_REGISTRA_UTENTE 'R'
#define MSG_MATRICE 'M'
#define MSG_TEMPO_PARTITA 'T' // Time to end game.
#define MSG_TEMPO_ATTESA 'A' // Time remaining to the start of a new game, pause left time.
#define MSG_PAROLA 'W'
#define MSG_PUNTI_FINALI 'F'
#define MSG_PUNTI_PAROLA 'P'

#define MSG_ESCI 'Q'
#define MSG_IGNORATO 'I'

#define BUFFER_SIZE 1024  // Size of the buffers that will be used in some cases.

// Struct of the message that will be in the communication between server and clients.
struct Message {
    char type;  // Type of message.
    unsigned int length; // Length of the data field.
    char* data;  // Message content.
};

struct sockaddr_in server_addr; // Socket server address.
int socket_server_fd; // Socket server file descriptor.

sigset_t signal_mask; // Signal mask to handle signals (SIGINT and SIGALRM).
pthread_t sig_thr_id; // Thread that will handle the signals (SIGINT and SIGALRM).

// Functions used both in client and server, their implementation normally is
// the same, is done in common.c.
int parseIP(char*, struct sockaddr_in*);
void toLowerOrUpperString(char*, char);

// Present both in client and server, but with DIFFERENT IMPLEMENTATION.
void clearExit(void);
void* signalsThread(void*);
////////////////////////////////////////////////////////////////////////

struct Message* receiveMessage(int);
void sendMessage(int, char, char*);
void destroyMessage(struct Message**);
void mLock(pthread_mutex_t*);
void mULock(pthread_mutex_t*);
void* readInterrupted(struct Message**);

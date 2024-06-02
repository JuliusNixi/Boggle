// Shared/Common CLIENT & SERVER cross files vars and libs.
#include "common.h"

// Common server only cross files used vars and libs.
int socket_server_fd; // Socket server file descriptor.

typedef unsigned long int uli; // Shortcut used below.
uli gameduration; // Game duration, for each match. 

int usematrixfile;  // 1 if a path has been specified by the user trought CLI args, 0 otherwise.
char* matpath; // String path rapresenting the path of the matrix file (specified by CLI arg).
// It's not allocated, it just point (after the initialization) to argv[1].

//------------------------------------------------------------------------------------
/*              REMEMBER                */
// These variables may not be used in the server.c file, but they will be used in some tests in the tests.c file.

// Numbers of columns and rows of the game matrix (default 4x4).
#define NROWS 4 // Matrix number of rows.
#define NCOL 4 // Matrix number of columns.

// Game matrix.
char matrix[NROWS][NCOL];  // Matrix game core, each position is a char.

#define VOID_CHAR '-' // Special char that will be used to indicate an undefined state.

#define ALPHABET "abdcdefghijklmnopqrstuvxyz" // Alphabet to be use to generate a random matrix and allowed chars for a client name.
//------------------------------------------------------------------------------------

// The player/client will be stored in a heap linked list, with the below structure.
// Used in functions signatures below.
struct ClientNode {
    int socket_client_fd;  // Client socket descriptor.

    struct sockaddr_in client_addr; // Client address.
    socklen_t client_address_len; // Client address length.

    struct ClientNode* next;  // Pointer of the next node of the list.

    pthread_t thread; // Thread that will handle the player/client.

    pthread_mutex_t handlerequest;  // Each client will have a mutex that will be acquired 
    // from the corresponding thread when a request will be taken over.
    // So, in case the game ends, the thread that will manage the pause
    // will wait for all clients to finish the requests received before.

    pthread_mutexattr_t handlerequestattr; // Used to change the above mutex type to PTHREAD_MUTEX_ERRORCHECK
    // to perform some checks that with the default would not be possible to do
    // for more info: https://man7.org/linux/man-pages/man3/pthread_mutex_lock.3p.html

    unsigned int points; // Player game points.

    char** words_validated; // To remember the already submitted words by the player.

    char* name; // Player's name.

    struct Message* queuedmessage; // Used to save a suspended message that need to be processed after an interrupt.

}; 


// Queue used only as required by the text to handle the end game.
/*

I chose the main data structure (the above linked list struct ClientNode*), to start working on
the project, prior to the pubblication of the full text of the project (using the slides),
so this queue poorly fits.
It is in fact, unnecessary and pedantic (more complicated than necessary) to use it to make threads
cooperate. Much more simply for the pause manager thread, is to block all threads via the
corresponding mutexes, going through the list, and it would have already had access to the
player's points information in a safe way to produce the final scoreboard.

Also, it would have sufficed that the queue contains the pointer to struct ClientNode*
(which contains the player's score), but since it is stated in the text of the project:

[...] ogni thread (che gestisce un client) spedisce un MESSAGGIO contenente il punteggio del client
su di una coda che e' condivisa tra i diversi thread. [...]

I thought it was explicitly required that the corresponding named data structure should be used
(struct Message), even though it does not, as it was described, contain the client's name,
so I went to enter the information user name, relative score, as a string in the data field
of the Message* struct.

In conclusion, I have much more complicated than necessary, but I have done so 
to remain as faithful as possible to the text of the project.


*/
struct Queue {
    struct ClientNode* client;
    struct Message* message;
    struct Queue* next;
};

// Functions signatures server used. Implementation and infos in the server main file.
void* scorer(void*);
char* csvNamePoints(struct Message*, int);
int sortPlayersByPointsMessage(const void*, const void*);
void generateRandomMatrix(void);
void loadMatrixFromFile(char*);
// Present both in client and server, but with DIFFERENT IMPLEMENTATION.
// void* signalsThread(void* args); -> common.h
// void atExit(void) -> common.h
// void threadDestructor(void*); -> common.h
// void signalsThreadDestructor(void*); -> common.h
// void makeKey(void); -> common.h
// void threadSetup(void); -> common.h
////////////////////////////////////////////////////////////////////////
void getMatrixNextIndexes(int*);
void validateMatrix(void);
void initMatrix(void);
char* serializeMatrixStr(void);
void loadDictionary(char*);
void validateDictionary(void);
int registerUser(char*, struct ClientNode*, struct Message*);
void setAlarm(void);
void startGame(void);
void* gamePause(void*);
void acceptClient(void);
// I used unsigned long because i read that is the recommended type for handling POSIX time.
char* itoa(uli);
uli timeCalculator(uli, char);
void sendCurrentMatrix(struct ClientNode*);
void* clientHandler(void*);
int submitWord(struct ClientNode*, char*);
int searchWordInMatrix(int, int, char*);
int validateWord(char*);
void disconnectClient(struct ClientNode**, int);
void endGame(int);
void updateClients(void);
char* serializeStrClient(struct ClientNode*);
void createScoreboard(struct Queue**, int);
void gameEndQueue(struct ClientNode*);
void clearQueue(void);
// int parseIP(char*, struct sockaddr_in*); -> common.h
// void toLowerOrUpperString(char*, char); -> common.h
// struct Message* receiveMessage(int); -> common.h
// void sendMessage(int, char, char*); -> common.h
// void destroyMessage(struct Message**); -> common.h
// void mLock(pthread_mutex_t*); -> common.h
// void mULock(pthread_mutex_t*); -> common.h
// void* readInterrupted(struct Message**); -> common.h
// void handleError(int, int, int, int, const char*, ...); -> common.h
// void printff(va_list, const char*, ...); -> common.h



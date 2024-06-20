// Shared/Common CLIENT & SERVER cross files vars and libs.
#include "common.h"

// Common server only cross files used vars and libs.
int socket_server_fd; // Socket server file descriptor.

uli gameduration; // Game duration, for each match, in minutes. 

char usematrixfile;  // 1 if a path has been specified by the user trought CLI args, 0 otherwise.
char* matpath; // String path rapresenting the path of the matrix file (specified by CLI arg). It's not allocated, it just point (after the initialization) to argv[1].

//------------------------------------------------------------------------------------
/*              REMEMBER                */
// These variables may not be used in the server.c file, but they are used in some tests in the Tests\tests.c file.

// Numbers of columns and rows of the game matrix (default 4x4).
#define NROWS 4 // Matrix number of rows.
#define NCOL 4 // Matrix number of columns.

// Game matrix.
char matrix[NROWS][NCOL];  // Matrix game core, each position is a char.

#define VOID_CHAR '-' // Special char that will be used to indicate an undefined state.

#define ALPHABET "abdcdefghijklmnopqrstuvxyz" // Alphabet used to generate a random matrix and allowed chars for a client name (regitration).
//------------------------------------------------------------------------------------

// The player/client will be stored in a heap linked list, with the below structure.
// Defined here (and not in server.c) because is used in functions signatures below.
struct ClientNode {
    int socket_client_fd;  // Client socket descriptor.

    struct sockaddr_in client_addr; // Client address.
    socklen_t client_address_len; // Client address length.

    struct ClientNode* next;  // Pointer of the next node of the list.

    pthread_t thread; // Thread that will handle the player/client.

    pthread_mutex_t handlerequest;  // Each client will have a mutex that will be acquired from the corresponding thread when a request will be taken over. So, in case of game ends, the thread that will manage the pause (signalsThread()) will wait for all clients threads (clientHandler()) to finish the requests RECEIVED BEFORE.

    pthread_mutexattr_t handlerequestattr; // Used to change the above mutex type to PTHREAD_MUTEX_ERRORCHECK. To perform some checks that with the default would not be possible to do.
    // For more info: https://man7.org/linux/man-pages/man3/pthread_mutex_lock.3p.html

    uli points; // Player game points, increased when the player guess a word.

    char** words_validated; // To remember the already submitted words by the player.

    char* name; // Player's name, must be composed of ALPHABET characters.

    struct Message* registerafter; // Used to save a suspended register request that need to be processed after the end game phase.

    char actionstoexecute; // This var is used to create a very simple "communication" between the signalsThread() thread and the clientHander() threads, without using others more complex synchronization primitives.

    // https://stackoverflow.com/questions/24931456/how-does-sig-atomic-t-actually-work
    volatile sig_atomic_t receivedsignal; // This is used by the clientHandler() threads to notify the signalsHandler() thread that the signal sent (SIGUSR1) was received.
    /* 

            NOTES

    Without this last field, an insidious bug can occur (  that made me crazy :(  ) that happens
    rarely but is fatal! 
    The signalsHandler() thread acquires the mutex of a clientHandler() thread (released by him 
    very shortly before), and sends the signal (SIGUSR1 of end game) to it.
    If unluckily the signal arrives exactly between the unlock of the mutex and the start of
    the read(), it is "lost".
    Lost means that the registered signal handler (which does nothing and returns)
    is actually executed, however, in this project, there was the central idea that a clientHandler()
    thread could stand by on a read() and have it interrupt via a signal, to force the
    clientHandler() thread to execute the endgame actions while minimizing the synchronization
    primitives that reduce the effectiveness of multithreading.
    My idea was that the clientHandler() thread would release its mutex and contextually wait
    right away on the read(), since there is no instruction between the two (unlock and read())
    and the other thread (signalsHandler()) has to go through the whole list of players to acquire
    all clients threads mutexes. 
    But obviously I was wrong, because yes, it is rare (but not too rare either if there are only
    a few connected clients), that the signalsHandler() is executed first, which sends the signal
    to the clientHandler() thread, which executes the handler, returns and only after that does
    it wait on the read(). And if the player does not send any requests the clientHandler() thread
    gets stuck on the read() indefinitely, does not perform the endgame action of writing to the
    queue, and the signalsHandler() thread waits for it potentially forever in a deadlock that
    blocks the whole game. 

    */

   char waiting; // This is used by the clientHandler() threads to notify the signalsHandler() thread that we are waiting on the handlerequest mutex. 

   char toexit; // This is used to notify the signalsThread() thread of a client's disconnection.

}; 


// Queue used only as required by the project text to handle the end game.
/*
        NOTES

I chose the main data structure (the above linked list struct ClientNode*), to start working on
the project, prior to the pubblication of the full text of the project (using the summary slides),
so this queue poorly fits.
It is in fact, unnecessary and pedantic (more complicated than necessary) to use it to make threads
cooperate. Much more simply for the pause manager thread (signalsHandler()), is to block all threads 
via the corresponding mutexes, going through the players list, and it would have already had access 
to the player's points information in a easy and safe way to produce the final scoreboard.

Also, it would have sufficed that the queue contains the pointer to struct ClientNode*
(which contains the player's score), but since it is stated in the text of the project:

[...] ogni thread (che gestisce un client) spedisce un MESSAGGIO contenente il punteggio del client
su di una coda che e' condivisa tra i diversi thread. [...]

I thought it was explicitly required that the corresponding named data structure should be used
(struct Message), even though it does not, as it was described, contain the client's name,
so I went to enter the information user name, relative score, as a string in the data field
of the Message* struct.

TL;DR: In conclusion, I have much more complicated than necessary, but I have done so 
to remain as faithful as possible to the text of the project.

*/
struct Queue {
    struct ClientNode* client;
    struct Message* message;
    struct Queue* next;
};

volatile sig_atomic_t* threadsignalreceivedglobal; // This is used by the clientHandler() threads to notify the signalsHandler() thread that the signal sent (SIGUSR1) was received.

// Functions signatures server used in server.c and bloggle_server.c.
// Implementation and infos in the server.c file.
void* scorer(void*);
char* csvNamePoints(struct Message*, char);
int sortPlayersByPointsMessage(const void*, const void*);
void generateRandomMatrix(void);
void loadMatrixFromFile(char*);
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
void disconnectClient(struct ClientNode**);
void endGame(int);
void updateClients(void);
char* serializeStrClient(struct ClientNode*);
void createScoreboard(struct Queue**, int);
void gameEndQueue(struct ClientNode*);
void clearQueue(void);


// Defined, commented and implemented all in common.h and common.c.
// int parseIP(char*, struct sockaddr_in*); -> common.h
// void toLowerOrUpperString(char*, char); -> common.h
// struct Message* receiveMessage(int); -> common.h
// void* sendMessage(int, char, char*); -> common.h
// void destroyMessage(struct Message**); -> common.h
// void mLock(pthread_mutex_t*); -> common.h
// void mULock(pthread_mutex_t*); -> common.h
// void handleError(char, char, char, char, const char*, ...); -> common.h
// void printff(va_list, const char*, ...); -> common.h
// void makeKey(void); -> common.h
// void threadSetup(void); -> common.h


// Present both in client and server, but with DIFFERENT IMPLEMENTATION.
// void* signalsThread(void*); -> common.h
// void atExit(void) -> common.h
// void threadDestructor(void*); -> common.h
////////////////////////////////////////////////////////////////////////






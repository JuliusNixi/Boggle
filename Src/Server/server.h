// Shared/Common CLIENT, SERVER & TESTS cross files vars and libs.
#include "../Common/common.h"

// Common server only cross files used vars and libs.
int socket_server_fd; // Socket server file descriptor.

uli gameduration; // Game duration, for each match, in minutes. 
uli pauseduration; // Duration of the pause in minutes. Default 1 minute.

char usematrixfile;  // 1 if a matrices file path has been specified by the user trought CLI args, 0 otherwise.
char* matpath; // String rapresenting the path of the matrices file (specified by CLI arg). It's not allocated, it just point (after the initialization) to the right argv.

// The player/client infos will be stored in a heap linked list, with the below structure.
// Defined here (and not in server.c) because is used in functions signatures below.
struct ClientNode {
        
    int socket_client_fd;  // Client socket descriptor.

    struct sockaddr_in client_addr; // Client address.
    socklen_t client_address_len; // Client address length.

    struct ClientNode* next;  // Pointer of the next node of the list.

    pthread_t thread; // Thread that will handle the player/client.
    char threadstarted; // This variable is 1 if the previous thread was started, 0 otherwise. 

    pthread_mutex_t handlerequest;  // Each client will have a mutex that will be acquired from the corresponding thread when a request will be taken over. So, in case of game ends, the thread that will manage the pause (signalsThread()) will wait for all clients threads (clientHandler()) to finish the requests RECEIVED BEFORE.

    uli points; // Player game points, increased when the player guess a word.

    char** words_validated; // To remember the already submitted words by the player, it's a copy of words_valid.

    char* name; // Player's name, must be composed of ALPHABET (common.h) characters.

    // Synchronization tools.
    struct Message* registerafter; // Used to save a suspended register request that need to be processed after the end game phase for synchronization reasons.
    char actionstoexecute; // This var is used to create a very simple "communication" between the signalsThread() thread and the clientHander() thread, without using others more complex synchronization primitives, to handle the game end.
    // https://stackoverflow.com/questions/24931456/how-does-sig-atomic-t-actually-work
    volatile sig_atomic_t receivedsignal; // This is used by the clientHandler() thread to notify the signalsHandler() thread that the signal sent (SIGUSR1) was received.
    uli countertimeoutseconds; // This is used to count seconds. When this value reach MESSAGE_TIMEOUT_SECONDS (defined in server.c) the unresponsive client is disconnected.
    // These three variables could be 1 or 0 only.
    char waiting; // This is used by the clientHandler() thread to notify the signalsHandler() thread that we are waiting on the handlerequest mutex. 
    char toexit; // This is used by the clientHandler() thread to notify the signalsThread() thread of a client's disconnection beginning.
    char filledqueue; // This is used by the clientHandler() thread to notify the signalsThread() thread of the client has correctly filled the queue.

}; 

// Queue used only as required by the project's text to handle the end game.
/*
        NOTES

It is in fact, unnecessary and pedantic (more complicated than necessary) to use it to make threads
cooperate. Much more simply for the pause manager thread (signalsHandler()), is to block all threads 
via the corresponding mutexes (handlerequest), by going through the players list, and it would have  
already had access to the player's points information in a easy and safe way to produce the final
scoreboard.

Also, it would have sufficed that the queue contains the pointer to struct ClientNode*
(which contains the player's score), but since it is stated in the text of the project:

[...] ogni thread (che gestisce un client) spedisce un MESSAGGIO contenente il punteggio del client
su di una coda che e' condivisa tra i diversi thread. [...]

I thought it was explicitly required that the corresponding named data structure should be used
(struct Message), even though it does not, as it was described in the project's text, contains
the client's name, so I went to enter the information name, relative score, as a string in the
data field of the Message* struct in "manual" way.

TL;DR: In conclusion, I have much more complicated than necessary, but I have done so 
to remain as faithful as possible to the project's text.

*/
struct Queue { // Queue struct.
    struct ClientNode* client; // Pointer to the client's infos struct.
    struct Message* message;  // Pointer to a message (type struct Message). It's type will be MSG_PUNTI_FINALI. The data field will contain a string in this format "playername,playerpoints".
    struct Queue* next; // Pointer to the next element of the Queue.
};

//#define TEST_MODE_SECONDS 8LU // This is used ONLY for testing to use time in seconds, not minute and do not wait a lot.

// Functions signatures server used in server.c and bloggle_server.c.
// Implementation and infos in the server.c file.
void loadDictionary(char*);
void getMatrixNextIndexes(int*);
void validateMatrix(void);
void loadMatrixFromFile(char*);
void generateRandomMatrix(void);
char* serializeMatrixStr(void);
void validateDictionary(void);
char searchWordInMatrix(uli, uli, char*);
void startGame(void);
void updateClients(void);
void acceptClient(void);
void disconnectClient(struct ClientNode**, char);
char* serializeStrClient(struct ClientNode*);
char processReceivedRequest(struct Message**, struct ClientNode*, char);
void gameEndQueue(struct ClientNode*);
char* csvNamePoints(struct Message*, char);
void* clientHandler(void*);
int registerUser(char*, struct ClientNode*, struct Message*);
uli timeCalculator(uli, char, char*);
int submitWord(struct ClientNode*, char*);
int validateWord(char*);
void endGame(int);
void* scorer(void*);
int sortPlayersByPointsMessage(const void*, const void*);
void createScoreboard(struct Queue**, uli);
void* gamePauseAndNewGame(void*);

// Defined, commented and implemented all in common.h and common.c.
// int parseIP(char*, struct sockaddr_in*); -> common.h
// void toLowerOrUpperString(char*, char); -> common.h
// struct Message* receiveMessage(int, char*); -> common.h
// char sendMessage(int, char, char*); -> common.h
// void destroyMessage(struct Message**); -> common.h
// char* bannerCreator(uli, uli, char*, char, char); -> common.h
// char* itoa(uli); -> common.h
// void disconnecterChecker(int*); -> common.h

// Present both in client and server, but with DIFFERENT IMPLEMENTATION.
// void* signalsThread(void*); -> common.h


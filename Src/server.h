// Shared/Common CLIENT & SERVER files vars and libs.
#include "common.h"

// Common server only files used vars and libs.
typedef unsigned long int uli;

uli gameduration; // Game duration. 

int usematrixfile;  // 1 if a path has been specified by the user thought CLI args, 0 otherwise.
char* matpath; // String path rapresenting the path of the matrix file (specified by CLI arg).
// It's not allocated, it just point (after the initialization) to argv[1].


/*              REMEMBER                */
// These variables may not be used in the server.c file, but they will be used in the tests.c file.

// Numbers of columns and rows of the game matrix (default 4x4).
#define NROWS 4 // Matrix number of rows.
#define NCOL 4 // Matrix number of columns.

// Game matrix.
char matrix[NROWS][NCOL];  // Matrix game core.

#define VOID_CHAR '-' // Special char that will used to indicate an undefined state.

#define ALPHABET "abdcdefghijklmnopqrstuvxyz" // Alphabet to be use to generate a random matrix and allowed chars for a client name.
//------------------------------------------------------------------------------------

// The player/client will be stored in a heap linked list, with the below structure.
struct ClientNode {
    int socket_client_fd;  // Client socket descriptor.
    struct sockaddr_in client_addr; // Client address.
    socklen_t client_address_len; // Client address length.
    struct ClientNode* next;  // Pointer of the next node of the list.
    pthread_t thread; // Thread that will handle the player/client.
    pthread_mutex_t handlerequest;  // Each client will have a mutex that will be acquired from the corresponding thread when a request will be taken over.
    pthread_mutexattr_t handlerequestattr;
    unsigned int points; // Player points.
    char** words_validated; // To remember the already submitted words by the player.
    char* name; // Player's name.
    struct Message* queuedmessage; // Used to save a suspended message that need to be processed.
    int exited; // Used to detected disconnected clients.
}; 

// Functions signature server used. Implementation and infos in the server main file.
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
// I used unsigned long because i read that is the best type for handling POSIX time.
char* itoa(uli);
uli timeCalculator(uli, char);
void sendCurrentMatrix(struct ClientNode*);
void* clientHandler(void*);
int submitWord(struct ClientNode*, char*);
int searchWordInMatrix(int, int, char*);
int validateWord(char*);
// Present both in client and server, but with DIFFERENT IMPLEMENTATION.
// void clearExit(void); -> common.h
// void* signalsThread(void* args); -> common.h
////////////////////////////////////////////////////////////////////////
// struct Message* receiveMessage(int); -> common.h
// void sendMessage(int, char, char*); -> common.h
// void destroyMessage(struct Message**); -> common.h
// int parseIP(char*, struct sockaddr_in*); -> common.h
// void toLowerOrUpperString(char*, char); -> common.h
void disconnectClient(struct ClientNode**, int);
void endGame(int);
void updateClients(void);
void printConnectedClients(struct ClientNode*);
void gameEndQueue(struct ClientNode*);
void clearQueue(void);
void* scorer(void*);

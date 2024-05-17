// Common used libs.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include <arpa/inet.h>
struct sockaddr_in server_addr; // Socket server address.
int socket_server_fd; // Socket server file descriptor.

// Functions used. Implementation and infos in the server.c file. 
void toLowerOrUpperString(char*, char);
int parseIP(char*, struct sockaddr_in*);
void startGame(void);
void acceptClient(void);
void sigintHandler(int);
void timerHandler(int);
void* clientHandler(void*);
void initMatrix(void);
void loadDictionary(char*);
void generateRandomMatrix(void);
void validateMatrix(void);
void getMatrixNextIndexes(int*);
char* serializeMatrixStr(void);
void loadMatrixFromFile(char*);
void setAlarm(void);
int searchWordInMatrix(int, int, char*);
void validateDictionary(void);
int validateWord(char*);

unsigned int duration; // Game duration.

/*              REMEMBER USED ALSO IN TEST FILE!                */
// Numbers of columns and rows of the game matrix (default 4x4).
#define NROWS 4 // Matrix number of rows.
#define NCOL 4 // Matrix number of columns.

// Game matrix.
char matrix[NROWS][NCOL];  // Matrix game core.

#define VOID_CHAR '-' // Special char that will used to indicate an undefined state.

#define ALPHABET "abdcdefghijklmnopqrstuvxyz" // Alphabet to be use to generate the matrix.


/*

NON IN USO

#define MSG_OK 'K'
#define MSG_ERR 'E'
#define MSG_REGISTRA_UTENTE 'R'
#define MSG_MATRICE 'M'
#define MSG_TEMPO_PARTITA 'T'
#define MSG_TEMPO_ATTESA 'A'
#define MSG_PAROLA 'W'
#define MSG_PUNTI_FINALI 'F'
#define MSG_PUNTI_PAROLA 'P'

struct Message {
    char type;
    unsigned int length;
    char* data;
};

#define MAX_NUM_CLIENTS 32

*/






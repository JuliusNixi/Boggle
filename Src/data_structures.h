// Number of columns and rows of the game matrix (default 4x4).
const int NROWS = 4;
const int NCOL = 4;

// Alphabet to be use to generate the matrix.
char alphabet[] = "ABCDEFGHILMNOPQRSTUVZ";

// Game matrix.
char** matrix = 0;
/*
    
    Below i calculate the string length.
    There are 4 addends enclosed in a parenthesis.
    The first is the length of the only data contained in the matrix 
    multiplied by 2 because every position could be Qu.
    The second addend is the number of spaces betwen letters.
    The third rapresent the \n at the end of each line.
    The fourth plus 1 is the string terminator (\0).
    
*/
#define matrixstrlength (NCOL * NROWS * 2) + ((NCOL - 1) * NROWS) + (NROWS) + (1)
char matrixstring[matrixstrlength];
int matrixint[NROWS][NCOL];

// maybe to change
#define BUFFER_SIZE 1024

#define VOID_CHAR 'X'

#include <netinet/in.h>
#include <pthread.h>
struct sockaddr_in server_addr;
int socket_server_fd;

struct ClientNode {
    int socket_client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_address_len;
    struct ClientNode* next;
    unsigned int id;
    pthread_t thread;
}; 
struct ClientNode* head;
struct ClientNode* tail;
pthread_mutex_t listmutex = PTHREAD_MUTEX_INITIALIZER;

int duration = 0;
struct sigaction sigactiontimer;
#define SIG_ALRM_ALERT "Time expired, the timer sounds!\n"
#include <string.h>
const size_t SIG_ALRM_ALERT_LEN = strlen(SIG_ALRM_ALERT);

#define DICT_PATH "../Data/dizionario.txt"
char** words;
size_t words_len;

// Numbers of columns and rows of the game matrix (default 4x4).
#define NROWS 4 // Matrix number of rows.
#define NCOL 4 // Matrix number of columns.

// Alphabet to be use to generate the matrix.
#define ALPHABET "ABCDEFGHILMNOPQRSTUVZ" // Alphabet used in the game.

// Game matrix.
char matrix[NROWS][NCOL];  // Matrix game core.
/*
    
    Below i calculate the string length of the matrix.
    There are 4 addends enclosed in a parenthesis.
    The first is the length of the only data contained in the matrix 
    multiplied by 2 because every position could be Qu.
    The second addend is the number of spaces betwen letters.
    The third rapresent the \n at the end of each line.
    The fourth plus 1 is the string terminator (\0).

    Is used to serialize the matrix into a visually pretty string.
    
*/
#define MAT_STR_LEN (NCOL * NROWS * 2) + ((NCOL - 1) * NROWS) + (NROWS) + (1) // Matrix serialized string length.
char matrixstring[MAT_STR_LEN]; // Matrix serialized string container.

#define BUFFER_SIZE 1024  // Size of the buffers that will be used.

#define VOID_CHAR 'X' // Special char that will used to indicate an undefined matrix state.

char* MAT_PATH = 0; // String that will be allocated on the heap and will rapresent the matrix file path (if present).

#include <netinet/in.h>
#include <pthread.h>
struct sockaddr_in server_addr; // Socket server address.
int socket_server_fd; // Socket server file descriptor.

// The player will be stored in a linked list, with the below structure.
struct ClientNode { // Data structure that will rapresent a player/client.
    int socket_client_fd;  // Client socket descriptor.
    struct sockaddr_in client_addr; // Client address.
    socklen_t client_address_len; // Client address length.
    struct ClientNode* next;  // Pointer of the next node.
    unsigned int id; // Unique player/client id.
    pthread_t thread; // Thread that will handle the player/client.
}; 
struct ClientNode* head; // Pointer to the client list head.
struct ClientNode* tail; // Pointer to the client list tail.
pthread_mutex_t listmutex = PTHREAD_MUTEX_INITIALIZER; // Mutex that will used to manage interactions with the list for threads.

unsigned int duration = 10; // Game duration, default 10 minutes.
struct sigaction sigactiontimer; // Timer that will handle the game time.
#define SIG_ALRM_ALERT "Time expired, the timer sounds!\n"
#include <string.h>
const size_t SIG_ALRM_ALERT_LEN = strlen(SIG_ALRM_ALERT);

#define DICT_PATH "../Data/dizionario2.txt"   // Path to the dictionary file that will be used to check if a word is valid.
char** words = 0;  // Pointer to a char[][] array that will be allocated on the heap. Each string rapresent a word/line on the dictionary file.
size_t words_len = 0;   // Length of the char[][] above.

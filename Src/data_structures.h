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

// maybe to change
#define BUFFER_SIZE 1024

#define VOID_CHAR 'X'

#include <netinet/in.h>
struct sockaddr_in socket_server;
int socket_server_fd;

struct ClientNode {
    struct sockaddr_in client_address;
    socklen_t client_address_len;
    int socket_client_fd;
    struct ClientNode* next;
};


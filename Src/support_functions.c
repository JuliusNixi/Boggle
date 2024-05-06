#include "data_structures.h"
#include "support_functions.h"
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define SIG_ALRM_ALERT "Time expired, the timer sounds!\n"
const size_t SIG_ALRM_ALERT_LEN = strlen(SIG_ALRM_ALERT);

// This function initialize the matrix (global char**) by allocating memory
// in the heap and filling it with a special character.
void initMatrix(void) {

    if (matrix != NULL) {
        // Error already initialized.
    }
    // Initializing rows.
    matrix = malloc(sizeof(char*) * NROWS);
    if (matrix == NULL) {
        // Error
    }
    for (int i = 0; i < NROWS; i++) {
        // Initializing columns.
        matrix[i] = malloc(sizeof(char) * NCOL);
        if (matrix[i] == NULL) {
            // Error
        }
    }
    // Initializing the matrix with a special character, useful for testing and debugging.
    for (int i = 0; i < NROWS; i++)
        for (int j = 0; j < NCOL; j++)
            matrix[i][j] = VOID_CHAR;

}

// This functions check if the matrix (global char**) is initialized.
// If it is not, it will be made.
void isInitialized(void) {

    if (matrix == NULL)
        initMatrix();

}

// This function generate a random letter matrix of size as written in NCOL and NROWS.
// The matrix is stored in the heap by using (global char**) pointer.
void generateRandomMatrix(void) {

    // Check the matrix existance.
    isInitialized();
    int randint;
    for (int i = 0; i < NROWS; i++)
        for (int j = 0; j < NCOL; j++) {
            // Choosing a random letter from alphabet (global var) and filling the matrix.
            randint = rand() % strlen(alphabet);
            matrix[i][j] = alphabet[randint];
        }

}

// This function is an iterator, useful to read/write the matrix.
// It takes as input an integer array of 2 positions (pointer to it).
// It fills array[0] with the matrix current row index.
// It fills array[1] with the matrix current column index.
void getMatrixNextIndexes(int* matrixnextindexes) {

    // Static indexes, persitent between function calls.
    static int i;
    static int j;
    if (matrixnextindexes == NULL) {
        // Initialization or reset of the iterator is made by passing NULL as arg.
        i = 0;
        j = 0;
        return;
    }
    // Generating next indexes.
    for (; i < NROWS; ) {
        for (; j < NCOL; ) {
            matrixnextindexes[0] = i;
            matrixnextindexes[1] = j++;
            if (j != NCOL)
                return;
            j = 0;
            break;
        }
        i++;
        return;
    }

    // Reset.
    // If we arrive here it means that we are trying to do an out of bound matrix access.
    // I report it by returning -1. Then i reset the iterator with a recursive call.
    matrixnextindexes[0] = -1;
    matrixnextindexes[1] = -1;
    getMatrixNextIndexes(NULL);

}

// This function check if the matrix content is legit in accordance with the alphabet.
// If is found in the matrix at least 1 charcater not present in the alphabet,
// the matrix is not valid, and a critical error is throw.
// If the matrix is valid, nothing happen.
void validateMatrix(void) {

    if (matrix == NULL) {
        // Error
    }

    for (int i = 0; i < NROWS; i++) {
        for (int j = 0; j < NCOL; j++){
            char c = matrix[i][j];
            int found = 0;
            // Searching char of matrix in the alphabet.
            for (int x = 0; x < strlen(alphabet); x++)
                if (c == alphabet[x]) {
                    found = 1;
                    break;
                }
            // Character not found, error.
            if (found == 0) {
                // Error
                printf("Invalid matrix\n");
                return;
            }
        }
    }

}

// This functions fills a global string that will rapresent the matrix.
void serializeMatrixStr(void) {

    // Checking the matrix validation.
    validateMatrix();

    // Initializing the string with a special character, useful for testing and debugging.
    int counter = 0;
    while (counter < matrixstrlength)
        matrixstring[counter++] = VOID_CHAR;

    // Inserting \n at the end of each row.
    int r = 0;
    counter = 0;
    while (counter < matrixstrlength) {
        // I use the module to understand when end of row in string is reached.
        if (r != 0 && r % ((NCOL * 2) + (NCOL - 1)) == 0) {
            matrixstring[counter] = '\n';
            r = 0;
            counter++;
            continue;
        }
        counter++;
        r++;
    }

    // Inserting letters.
    counter = 0;
    int letters = 0;
    // Iterator initialized.
    getMatrixNextIndexes(NULL);
    int matrixnextindexes[2];
    while (counter < matrixstrlength) {
        // End of line reached.
        if (matrixstring[counter] == '\n') {
            letters = 0;
            counter++;
            continue;
        }
        // Inserting next matrix letter in the string.
        if (letters % 3 == 0 && counter + 1 != matrixstrlength) {
            getMatrixNextIndexes(matrixnextindexes);
            matrixstring[counter] = matrix[matrixnextindexes[0]][matrixnextindexes[1]];
            if (matrixstring[counter] == 'Q') {
                matrixstring[++counter] = 'u';
                ++letters;
            }
        }
        letters++;
        counter++;
    }

    // Inserting spaces.
    counter = 0;
    while (counter < matrixstrlength) {
        if (matrixstring[counter] == VOID_CHAR && counter + 1 != matrixstrlength)
            matrixstring[counter] = ' ';
        counter++;
    }

    // Insert string terminator.
    matrixstring[matrixstrlength - 1] = '\0';

}

// This function generate a matrix of size as written in NCOL and NROWS by reading a file
// passed by arg.
void loadMatrixFromFile(char* path) {

    // Check the matrix existance.
    isInitialized();

   // Stat to get file information. retvalue to check syscalls returns.
   struct stat s;
   int retvalue;

   // Performing stat on file.
   retvalue = stat(path, &s);
   if (retvalue == -1) {
    // Error
   }
   // Check if the file is regular.
   if(!S_ISREG(s.st_mode)){
        // Error not a regular file
    }

    // To store file content.
    char file[s.st_size];

    // Opening the file in readonly mode.
    int fd = open(path, O_RDONLY, NULL);
    if (fd == -1) {
        // Error
    }

    // Reading the file content using a buffer of BUFFER_SIZE length.
    char buffer[BUFFER_SIZE];
    int counter = 0;
    while (1) {
        retvalue = read(fd, buffer, BUFFER_SIZE);
        if (retvalue == -1) {
            // Error
        }
        // Exit, end of file reached.
        if (retvalue == 0) break;

        // Copying the buffer in the main array.
        for (int i = 0; i < retvalue; i++)
            file[counter++] = buffer[i];
    }

    // Applying the read matrix from file to the program matrix var.
    // Initializing iterator.
    getMatrixNextIndexes(NULL);
    int matrixnextindexes[2];
    counter = 0;
    // i is static, so i can remember between functions call where
    // i've come to read.
    static int i = 0;
    // If i equals to the file size, i am at the end of it and I start reading all over again.
    if (i == s.st_size) i = 0;
    for (; i < s.st_size; i++) {
        // Skipping spaces, and 'u' of 'Qu'.
        if (file[i] == ' ' || file[i] == 'u') continue;
        if (file[i] == '\n') {
            getMatrixNextIndexes(matrixnextindexes);
            if (counter != NROWS * NCOL || matrixnextindexes[0] != -1) {
                // Error file format
                printf("Error file\n");
            }
            i++;
            break;
        }
        // Getting next matrix indexes to write.
        getMatrixNextIndexes(matrixnextindexes);
        if (matrixnextindexes[0] == -1){
            // Error
        }else {
            matrix[matrixnextindexes[0]][matrixnextindexes[1]] = file[i];
            counter++;
        }
    }

    // Validating the new matrix.
    validateMatrix();

}

// Merges two subarrays of arr[].
// First subarray is arr[l..m].
// Second subarray is arr[m+1..r].
void mergeChar(char arr[], int l, int m, int r)
{
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;

    // Create temp arrays.
    char L[n1], R[n2];

    // Copy data to temp arrays L[] and R[].
    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];

    // Merge the temp arrays back into arr[l..r].
    i = 0;
    j = 0;
    k = l;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        }
        else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    // Copy the remaining elements of L[],
    // if there are any.
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    // Copy the remaining elements of R[],
    // if there are any.
    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }

}

// l is for left index and r is right index of the
// sub-array of arr to be sorted.
void mergeSortCharK(char arr[], int l, int r)
{
    
    if (l < r) {

        int m = l + (r - l) / 2;

        // Sort first and second halves.
        mergeSortCharK(arr, l, m);
        mergeSortCharK(arr, m + 1, r);

        mergeChar(arr, l, m, r);

    }
    
}

// Wrapper for mergeSortK, to fix \0 character.
// Merge sort string characters.
void mergeSortChar(char arr[], int l, int r)  {
    
    int arr_size = r + 1;
    mergeSortCharK(arr, l, r);
  
    int counter = 1;
    for (int i = 0; i < arr_size; i++) {
        if (i == arr_size - 1) break;
        arr[i] = arr[counter++];
    }
    
    arr[arr_size - 1] = '\0';
    
}


//char arr[] = "ciao";
//int arr_size = sizeof(arr) / sizeof(arr[0]);
//mergeSort(arr, 0, arr_size - 1);


// An iterative binary search function on string characters.
int binarySearch(char arr[], int l, int r, char element)
{
    while (l <= r) {
        int m = l + (r - l) / 2;

        // Check if element is present at mid.
        if (arr[m] == element)
            return m;

        // If element greater, ignore left half.
        if (arr[m] < element)
            l = m + 1;

        // If element is smaller, ignore right half.
        else
            r = m - 1;
    }

    // If we reach here, then element was not present.
    return -1;
}

// binarySearch(arr, 0, n - 1, x);

// This function will wait a new client connection.
// It will accept it, will create a new client-list node on the heap.
// And finally will start a new pthread to handle the new client.
void acceptClient(void) {

    // Allocating heap memory for a new client node.
    struct ClientNode* new = NULL;
    new = (struct ClientNode*) malloc(sizeof(struct ClientNode));
    if (new == NULL) {
        // Error
    }

    // Waiting for a new client connection.
    new->client_address_len = (socklen_t) sizeof(new->client_addr);
    while (1) {
        new->socket_client_fd = accept(socket_server_fd, (struct sockaddr*) (&(new->client_addr)), &(new->client_address_len));
        if (new->socket_client_fd == -1) {
            // Error
            printf("Error in accepting client or a signal has been managed.\n");
        }else
            break;
    }

    // Updating global vars head and tail useful to manage the list.
    /*
    
    Note that the tail pointer can greatly improve the efficiency/performance of the
    program in the case of so many clients and therefore a long list.
    Since there is no need to traverse the list until the end, blocking the program flow
    and making impossible the acceptance of new clients for a long time.

    WARNING: The list data structure is shared by all the threads.
    It then becomes necessary to change it to mutual exclusion, otherwise
    race conditions could occur, for example if this thread were to add an item
    to the list and was suspended to run one that is trying to delete the last item.
    
    */
    new->next = NULL;
    // Lock the mutex.
    pthread_mutex_lock(&listmutex);
    if (head == NULL) {
        new->id = 0;
        head = new;
        tail = new;
    }else {
        tail->next = new;
        unsigned int lastid = tail->id;
        tail = tail->next;
        tail->id = ++lastid;
    }
    // Unlock the mutex.
    pthread_mutex_unlock(&listmutex);

    // Starting a new pthread to handle the new client.
    // The executed function will be clientHandler.
    // It receives as input the pointer to the node (of its client to be managed)
    // of the clients list.
    int retvalue = pthread_create(&(new->thread), NULL, clientHandler, new);
    if (retvalue != 0) {
        // Error
    }


    // CONTINUE HERE

    /* // QUI
    // Ricezione del messaggio
    ssize_t n_read = 0;
    char buffer[BUFFER_SIZE];
    n_read =  read(new->socket_client_fd, buffer, BUFFER_SIZE);
    printf("\n%s\n",buffer);
    */


}

// This function will run in a thread and will manage the client requests.
void* clientHandler(void* voidclient) {

    struct ClientNode* client = (struct ClientNode*) voidclient;
    printf("Cliend ID: %u.\n", client->id);

    return NULL;

}

// This function will manage the SIGALRM signal triggered by the timer.
// WARNING: The functions executed inside and vars used should be Async-Signal Safe
// to not interfere with the previously running function. If it happens, in this situation,
// the suspended accept client function will fail with -1 (TESTED!).
// But in this case is not problematic, because we will go, thanks to while, again on the
// accept function.
void timerHandler(int signum) {

    //write(STDOUT_FILENO, SIG_ALRM_ALERT, SIG_ALRM_ALERT_LEN);
    //pid_t pid = fork();
    return;

}

// This function simply set a timer using global var duration to handle the game time.
void setAlarm(void) {

    // Alarm takes as input seconds, but the user input is in minutes.
    alarm(60 * duration);
    printf("The game duration is now setted to %d minutes.\n", duration);
    printf("TEST: %d\n", alarm(0));

}

void startGame(void) {

    // loadMatrixFromFile(filepath);
    setAlarm();

}

/*
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include "macro.h"
#include <stdlib.h>
#include <stdio.h>


# Client ip e porta
INET_ADDRSTRLEN size of buffer
char buf [INET_ADDRSTRLEN] = {0}; 

         #include <arpa/inet.h>
	inet_ntop(AF_INET, &(new->client_addr.sin_addr), buf, new->client_address_len);
    printf("%d %s\n", new->client_addr.sin_port, buf);


*/



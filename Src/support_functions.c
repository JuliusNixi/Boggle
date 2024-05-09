#include "data_structures.h"
#include "support_functions.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

// This function initialize the game matrix (global char[NROWS][NCOL]).
// The matrix is a char** that will contain the game characters.
// Each matrix slot will rapresent a char of the game.
// The game Qu is rapresented only with Q to save space.
void initMatrix(void) {
      
    // Initializing the matrix with a special symbol, useful for testing and debugging.
    for (unsigned int i = 0; i < NROWS; i++)
        for (int j = 0; j < NCOL; j++) 
            matrix[i][j] = VOID_CHAR;
    
    printf("Game matrix succesfully initialized.\n");

}

// This function generate a random letter matrix of size as written in NCOL and NROWS.
// The matrix is a global var, that will be filled.
void generateRandomMatrix(void) {

    int randint;
    for (unsigned int i = 0; i < NROWS; i++)
        for (unsigned int j = 0; j < NCOL; j++) {
            // Choosing a random letter from alphabet (global var) and filling the matrix.
            randint = rand() % strlen(ALPHABET);
            matrix[i][j] = ALPHABET[randint];
        }
    printf("New random matrix created succesfully.\n");

}

// This function is an iterator, useful to read/write the matrix.
// It takes as input an integer array of 2 positions (pointer to it).
// It fills array[0] with the matrix current row index.
// It fills array[1] with the matrix current column index.
void getMatrixNextIndexes(int* matrixnextindexes) {

    // Static indexes, persitent between function calls.
    static unsigned int i;
    static unsigned int j;
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
    // I report it by returning -1. Then I reset the iterator with a recursive call.
    matrixnextindexes[0] = -1;
    matrixnextindexes[1] = -1;
    getMatrixNextIndexes(NULL);

}

// This function check if the matrix content is legit in accordance with the alphabet.
// If is found in the matrix at least 1 charcater not present in the alphabet,
// the matrix is not valid, and a critical error is throw.
// If the matrix is valid, nothing happen.
// The alphabet is a global #define.
void validateMatrix(void) {

    for (unsigned int i = 0; i < NROWS; i++) {
        for (unsigned int j = 0; j < NCOL; j++){
            char c = matrix[i][j];
            unsigned int found = 0;
            // Searching char of matrix in the alphabet.
            for (unsigned int x = 0; x < strlen(ALPHABET); x++)
                if (c == ALPHABET[x]) {
                    found = 1;
                    break;
                }
            // Character not found, error.
            if (found == 0) {
                // Error
                printf("Error invalid character %c in the matrix.\n", c);
                return;
            }
        }
    }

}

// This functions fills a global string that will rapresent visually the matrix.
void serializeMatrixStr(void) {

    // Checking the matrix validation.
    validateMatrix();

    // Initializing the string with a special character, useful for testing and debugging.
    unsigned int counter = 0;
    while (counter < MAT_STR_LEN)
        matrixstring[counter++] = VOID_CHAR;

    // Inserting \n at the end of each row.
    int r = 0;
    counter = 0;
    while (counter < MAT_STR_LEN) {
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
    unsigned int letters = 0;
    // Iterator initialized.
    getMatrixNextIndexes(NULL);
    int matrixnextindexes[2];
    while (counter < MAT_STR_LEN) {
        // End of line reached.
        if (matrixstring[counter] == '\n') {
            letters = 0;
            counter++;
            continue;
        }
        // Inserting next matrix letter in the string.
        if (letters % 3 == 0 && counter + 1 != MAT_STR_LEN) {
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
    while (counter < MAT_STR_LEN) {
        if (matrixstring[counter] == VOID_CHAR && counter + 1 != MAT_STR_LEN)
            matrixstring[counter] = ' ';
        counter++;
    }

    // Insert string terminator.
    matrixstring[MAT_STR_LEN - 1] = '\0';

}

// This function fill a matrix of size as written in NCOL and NROWS by reading a file
// passed by arg (with file path) received from CLI.
// By passing a valid path file as argument, the function will start to read matrices line
// by line (one matrix for file line) to the end of file.
// If NULL is passed as arg, the function will load the next
// matrix from the current file, otherwise will start from the first line of the new file passed.
void loadMatrixFromFile(char* path) {

    // i is static, so i can remember between functions call where
    // where I was left to read the file.
    static unsigned int i = 0;
    // Resetting i while a (new) path is received.
    if (path != NULL) {
        i = 0;
        // Releasing old path.
        if (MAT_PATH != NULL) free(MAT_PATH);
        // Allocating new memory for the new path.
        MAT_PATH = (char*) malloc(strlen(path) * sizeof(char));
        if (MAT_PATH == NULL) {
            // Error
            printf("Error in allocating heap memory for the matrices file path.\n");
        }
        // Copying the path to the global var.
        strcpy(MAT_PATH, path);
    }

   // Stat to get file information. retvalue to check syscalls returns.
   struct stat s;
   int retvalue;

   // Performing stat on file.
   retvalue = stat(MAT_PATH, &s);
   if (retvalue == -1) {
    // Error
    printf("Error in getting %s matrices file information.\n", MAT_PATH);
   }
   // Check if the file is regular.
   if(!S_ISREG(s.st_mode)){
        printf("Error %s matrices file is not a regular file.\n", MAT_PATH);
        // Error not a regular file
    }

    // To store file content.
    char file[s.st_size];

    // Opening the file in readonly mode.
    int fd = open(MAT_PATH, O_RDONLY, NULL);
    if (fd == -1) {
        // Error
        printf("Error in opening %s matrices file.\n", MAT_PATH);
    }

    // Reading the file content using a buffer of BUFFER_SIZE length.
    char buffer[BUFFER_SIZE];
    unsigned int counter = 0;
    while (1) {
        retvalue = read(fd, buffer, BUFFER_SIZE);
        if (retvalue == -1) {
            // Error
            printf("Error in reading %s matrices file.\n", MAT_PATH);
        }
        // Exit while, end of file reached.
        if (retvalue == 0) break;

        // Copying the buffer in the main file array.
        for (unsigned int i = 0; i < retvalue; i++)
            file[counter++] = buffer[i];
    }

    // Applying the read matrix from file to the program matrix var.
    // Initializing iterator.
    getMatrixNextIndexes(NULL);
    int matrixnextindexes[2];
    counter = 0;
    // If i equals to the file size, I am at the end of it and I start reading all over again.
    if (i == s.st_size) i = 0;
    for (; i < s.st_size; i++) {
        // Skipping spaces, and 'u' of 'Qu'.
        if (file[i] == ' ' || file[i] == 'u') continue;
        if (file[i] == '\n') {
            getMatrixNextIndexes(matrixnextindexes);
            if (counter != NROWS * NCOL || matrixnextindexes[0] != -1) {
                // Error file format
                printf("Error %s matrices file is in invalid format.\n", MAT_PATH);
            }
            i++;
            break;
        }
        // Getting next matrix indexes to write.
        getMatrixNextIndexes(matrixnextindexes);
        if (matrixnextindexes[0] == -1){
            // Error 
            printf("Error %s matrices file is in invalid format.\n", MAT_PATH);
        }else
            matrix[matrixnextindexes[0]][matrixnextindexes[1]] = file[i];
        
        counter++;
    }

    // Validating the new matrix.
    validateMatrix();

    printf("New matrix succesfully loaded from %s matrix file.\n", MAT_PATH);

}

// This function will wait a new client connection.
// It will accept it, will create a new client-list node on the heap.
// And finally will start a new pthread to handle the new client.
void acceptClient(void) {

    // Allocating heap memory for a new client node.
    struct ClientNode* new = NULL;
    new = (struct ClientNode*) malloc(sizeof(struct ClientNode));
    if (new == NULL) {
        // Error
        printf("Error in allocating heap memory for a new player client.\n");
    }

    // Waiting for a new client connection.
    new->client_address_len = (socklen_t) sizeof(new->client_addr);
    while (1) {
        new->socket_client_fd = accept(socket_server_fd, (struct sockaddr*) (&(new->client_addr)), &(new->client_address_len));
        if (new->socket_client_fd == -1) {
            // Error
            printf("Error in accepting a new client or a signal has been managed.\n");
        }else
            break;
    }
    printf("New client accepted.\n");

    // Updating global vars head and tail useful to manage the list.
    /*
    
    Note that the tail pointer can greatly improve the efficiency/performance of the
    program in the case of so many clients and therefore a long list.
    Since there is no need to traverse the list until the end, blocking the program flow
    and making impossible the acceptance of new clients for a long time.

    WARNING: The list data structure is shared by all the threads.
    It then becomes necessary to change it in a mutual exclusion, otherwise
    race conditions could occur, for example if this thread were to add an item
    to the list and was suspended to run one that is trying to delete the last item.
    
    */
    new->next = NULL;
    // Lock the mutex.
    int retvalue;
    retvalue = pthread_mutex_lock(&listmutex);
    if (retvalue != 0) {
        // Error
        printf("Error in acquiring list mutex.\n");
    }
    // Empty list.
    if (head == NULL) {
        new->id = 0;
        head = new;
        tail = new;
    }else {
        // Not empty list.
        tail->next = new;
        unsigned int lastid = tail->id;
        tail = tail->next;
        tail->id = ++lastid;
    }
    // Unlock the mutex.
    retvalue = pthread_mutex_unlock(&listmutex);
    if (retvalue != 0) {
        // Error
        printf("Error in releasing list mutex.\n");
    }
    printf("New client added.\n");
    
    // Starting a new pthread to handle the new client.
    // The executed function will be clientHandler.
    // It receives as input the pointer to the node (of its client to be managed)
    // of the clients list.
    retvalue = pthread_create(&(new->thread), NULL, clientHandler, new);
    if (retvalue != 0) {
        // Error
        printf("Error in starting a new client handler thread.\n");
    }
    printf("New client thread started.\n");

}

// This function will run in a separate thread and will manage the client requests.
// Each client is served by a thread.
// The returned object and the input are void*.
// Must be converted to the correct type.
void* clientHandler(void* voidclient) {

    // Casting void* to ClientNode*.
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

// This function simply set a POSIX timer using the global var duration, to handle the game time.
void setAlarm(void) {

    // Alarm takes as input seconds, but the user input is in minutes.
    alarm(60 * duration);
    printf("The game duration is now setted to %d minutes.\n", duration);

}
 // This function start a new game.
void startGame(void) {

    // printing new game started
    // loadMatrixFromFile(filepath) || generaterandommatrix();
    // print new matrix
    // setAlarm();
    // pause

}

// This function allocate and load in memory a char array[][].
// Each line is a char* to a word of the dictionary file.
// Assuming the dictionary file contains one word at each line.
// Dictionary file is used to check if a word submitted by a client is legit.
void loadDictionary(void) {

    if (words != NULL) {
        // Error
        printf("Error, dictionary %s already loaded.\n", DICT_PATH);
    }

   // Stat to get file information. retvalue to check syscalls returns.
   struct stat s;
   int retvalue;

   // Performing stat on file.
   retvalue = stat(DICT_PATH, &s);
   if (retvalue == -1) {
    // Error
    printf("Error in getting %s dictionary file informations.\n", DICT_PATH);
   }
   // Check if the file is regular.
   if(!S_ISREG(s.st_mode)){
        // Error not a regular file
        printf(" %s dictionary is not a regular file.\n", DICT_PATH);
    }

    // To store file content.
    char file[s.st_size];

    // Opening the file in readonly mode.
    int fd = open(DICT_PATH, O_RDONLY, NULL);
    if (fd == -1) {
        // Error
        printf("Error in opening %s dictionary file.\n", DICT_PATH);
    }

    // Reading the file content using a buffer of BUFFER_SIZE length.
    char buffer[BUFFER_SIZE];
    unsigned int counter = 0;
    while (1) {
        retvalue = read(fd, buffer, BUFFER_SIZE);
        if (retvalue == -1) {
            // Error
            printf("Error in reading %s dictionary file.\n", DICT_PATH);
        }
        // Exit while, end of file reached.
        if (retvalue == 0) break;

        // Copying the buffer in the main file array.
        for (unsigned int i = 0; i < retvalue; i++)
            file[counter++] = buffer[i];
    }

    // Counting file lines and allocating heap space.
    counter = 0;
    for (unsigned int i = 0; i < s.st_size; i++) {
        if (file[i] == '\n' || i + 1 == s.st_size)
            counter++;
    }
    words_len = counter;
    words = (char**) malloc(sizeof(char*) * words_len);
    if (words == NULL) {
        // Error
        printf("Error in allocating heap space for the dictionary file content.\n");
    }

    // Array counter.
    counter = 0;
    // Word length counter.
    unsigned int wl = 0;
    // Word character copying counter.
    unsigned int c = 0;
    // Reading the file content through the file char[].
    for (unsigned int i = 0; i < s.st_size; i++) {
        // A word is detecting when a \n is reached or the end of file is reached.
        if (file[i] == '\n' || i + 1 == s.st_size) {
            // Calculating word length.
            // +1 for the string terminator. +2 for the last char and the string terminator.
            int l = (file[i] == '\n' ? wl + 1 : wl + 2);
            // Allocating space for the new word.
            words[counter++] = (char*) malloc(sizeof(char) * l);
            if (words[counter - 1] == NULL) {
                // Error
                printf("Error in allocating heap space for, a word of the dictionary file.\n");
            }
            c = 0;
            // Copying the word (char by char) in the allocated space if it is not '\n'.
            for (unsigned int j = i - wl; j <= i; j++) 
                if (file[j] != '\n') words[counter - 1][c++] = file[j];
            // Inserting string terminator.
            words[counter - 1][c] = '\0';
            wl = 0;
        }  else 
            wl++;
    }

    printf("Words dictionary succesfully loaded from %s file.\n", DICT_PATH);

}

// Search a word in the dictionary file.
// word is the string to search in the dictionary.
// return 1 if find, 0 otherwise.
int validateDictionary(char* word) {

    for (int i = 0; i < words_len; i++) 
        if (strcmp(words[i], word) == 0) return 1;
    return 0;

}













// Search a word submitted by a player/client in the game matrix.
// returns 1 if the word is founded in the dictionary AND in the game matrix.
// if one of both fail the returns will be 0.
int searchInMatrix(char* word) {

    // #include <ctype.h> toupper() case sensitive?
    // controllo facoltativo parola deve essere di lunghezza fissata
    // if (strlen(word) != NROWS) return 0;

    if (validateDictionary(word) == 0) return 0;
    
    for (int i = 0; i < NROWS; i++)
        for (int j = 0; j < NCOL; j++) {
            // TODO
        }

    return 0;

}

char** copyMatrix(void) {

    char** c;
    c = (char**) malloc(sizeof(char*) * NROWS);
    for (int i = 0; i < NCOL; i++)
        c[i] = (char*) malloc(sizeof(char) * NCOL);
    for (int i = 0; i < NROWS; i++) {
        for (int j = 0; j < NCOL; j++)
            c[i][j] = matrix[i][j];
    }
    return c;

}




int check(int i, int j, char* s, char** tmpmatrix) {

   if (tmpmatrix == NULL) // Error ();
   if (s == NULL) return 0;
   if (i < 0 || i > NROWS - 1 || j < 0 || j > NCOL - 1) return 0;
    char c = s[0];
    if (c == '\0'){
        printf("TROVATO %d %d\n",i,j);
        return 1;
    } 
    printf("cerco %c elemento %c i: %d  j: %d -> %d\n",c, tmpmatrix[i][j],i,j,(int)c);
    if (tmpmatrix[i][j] == c) {
        s++;
        tmpmatrix[i][j] = 'X';
        // shortcut
        return check(i - 1, j, s,tmpmatrix) || check(i, j - 1, s,tmpmatrix) || check(i, j + 1, s,tmpmatrix) || check(i + 1, j, s,tmpmatrix);
    }
    return 0;

}

int cerca(char* word) {

    char** c = copyMatrix();
    for (int i = 0; i < NROWS; i++) {
        for (int j = 0; j < NCOL; j++) {
            printf("Matrice char for: %c \n", matrix[i][j]);
            int x = check(i,j, word,c);
            printf("Valore %d \n", x);
            c = copyMatrix();
        }
    }
    return 0;
}

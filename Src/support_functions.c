#include "data_structures.h"
#include "support_functions.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

// This function initialize the game matrix (global char[NROWS][NCOL]).
// Is not mandatory but recommended, to avoid annoying bugs during the development.
// The matrix is a char** that will contain the game characters.
// Each matrix slot will rapresent a char of the game.
// The symbol Qu is rapresented only with Q to save space.
void initMatrix(void) {
      
    // Initializing the matrix with a special symbol (VOID_CHAR), useful for testing and debugging.
    for (unsigned int i = 0; i < NROWS; i++)
        for (unsigned int j = 0; j < NCOL; j++) 
            matrix[i][j] = VOID_CHAR;
    
    printf("Game matrix succesfully initialized.\n");

}

// This function generate a random letters matrix of size as written in NCOL and NROWS.
// The matrix (global char[NROWS][NCOL]) will be filled with these letters.
void generateRandomMatrix(void) {

    int randint;
    // Iterating on game matrix.
    for (unsigned int i = 0; i < NROWS; i++)
        for (unsigned int j = 0; j < NCOL; j++) {
            // Choosing a random letter from alphabet (global #define) and filling the matrix.
            randint = rand() % strlen(ALPHABET);
            // The characters written in the alphabet are used all in UPPERCASE version,
            // regardless how hey are written in the #define global alphabet.
            matrix[i][j] = toupper(ALPHABET[randint]);
        }

    // Validating the new game matrix.
    validateMatrix();

    printf("New random matrix created and validated succesfully.\n");

}

// This function is an iterator, useful to read/write the matrix in complex functions.
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
    // I report it by returning -1. Then I reset the iterator with a recursive call passing NULL.
    matrixnextindexes[0] = -1;
    matrixnextindexes[1] = -1;
    getMatrixNextIndexes(NULL);

}

// This function check if the current matrix content is legit in accordance with the alphabet.
// If is found in the matrix at least 1 character not present in the alphabet,
// the matrix is not valid, and a critical error is throw.
// If the matrix is valid, nothing happen.
// The alphabet is defined as a global #define.
// This is useful to immediately notice any developmental errors that could lead
// the matrix into an inconsistent state.
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
            }
        }
    }

}

// This function fills a global string (char matrixstring[])
// It's length is defined by global #define calculated from NROWS and NCOLS data.
// The string will rapresent visually the game matrix.
// The Qu will be show in the result even if only the character Q is stored in the game matrix.
void serializeMatrixStr(void) {

    // Checking the matrix validation.
    validateMatrix();

    // Initializing the string with a special character, useful for testing and debugging.
    unsigned int counter = 0;
    while (counter < MAT_STR_LEN) matrixstring[counter++] = VOID_CHAR;

    // Inserting \n at the end of each row.
    unsigned int r = 0;
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

// This function fill the game matrix (global char[NROWS][NCOL]) of size as written in NCOL and NROWS
// by reading a file (with file path) received from CLI.
// By passing a valid path file as argument, the function will start to read matrices line
// by line (one matrix for file line ended with \n) beginning from the first to the end of file.
// If NULL is passed as arg, the function will load the next
// matrix from the current file, otherwise will start from the first line of the new file passed.
// The current used file path is stored on the heap with a char* global var MAT_PATH.
// The matrix will be loaded with all UPPERCASE characters regardless of how the characters
// are written in the file.
// If the matrices read from the file run out, faced with a new function call with a NULL argument,
// the function will start over in a "circular" way from the first line of the same file.
void loadMatrixFromFile(char* path) {

    // i is static, so i can remember between functions call
    // where I was left to read the file.
    static unsigned int i = 0;
    // Resetting i while a (new) path is received.
    if (path != NULL) {
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
        i = 0;
    }else
        // First call with NULL path.
        if (MAT_PATH == NULL) {
            // Error
            printf("Error, loadMatrixFromFile() has not been initialized. Recall it with a valid file path.\n");
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
            // The matrix are loaded with all characters UPPERCASE, regardless how
            // they are written in the file.
            matrix[matrixnextindexes[0]][matrixnextindexes[1]] = toupper(file[i]);
        
        counter++;
    }

    // Validating the new matrix.
    validateMatrix();

    printf("New matrix succesfully loaded and validated from %s matrix file.\n", MAT_PATH);

}

// This function will wait a new client connection.
// It will accept it when present, will create a new client-list node on the heap.
// Finally will start a new pthread to handle the new client.
// ClientNode is a global struct, .
void acceptClient(void) {

    // Allocating heap memory for a new client node.
    struct ClientNode* new = NULL;
    new = (struct ClientNode*) malloc(sizeof(struct ClientNode));
    if (new == NULL) {
        // Error
        printf("Error in allocating heap memory for a new player/client.\n");
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

    // Adding the client to the list and updating global vars head and tail useful
    // to manage the list.

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
// The default duration is 3 minutes. The duration can also be inserted by a CLI arg.
void setAlarm(void) {

    // Alarm takes as input seconds, but the user input is in minutes.
    alarm(60 * duration);
    printf("The game duration is now setted to %u minutes.\n", duration);

}

 // This function start a new game.
void startGame(void) {

    // printing new game started
    // loadMatrixFromFile(filepath) || generaterandommatrix();
    // print new matrix
    // if ... validateDictionary() because the current matrix is changed
    // setAlarm();
    // pause

}

// This function allocate and load in memory on the heap a global char array[][] called "words".
// Each line is a char* to a word of the dictionary file.
// Assuming the dictionary file contains one word at each line terminated by \n.
// Dictionary file CAN be used to check if a word submitted by a client is legit.
// It used only when the optional --diz is setted by CLI.
// So this function is not mandatory to use. If no dictionary has been received,
// this function will be useless and the game will only check if the word obtained
// from the client is present in the current game matrix.
void loadDictionary(char* path) {

    // Dictionary already loaded.
    if (words != NULL) {
        // Error
        //printf("Error, a dictionary has been already loaded, it can't be changed.\n");
        for (int i = 0; i < words_len; i++)
            free(words[i]);
        //TODO
        
    }

    // Empty path.
    if (path == NULL) {
        // Error
        printf("Error, loadDictionary(char* path) received an empty path.\n");
    }

   // Stat to get file information. retvalue to check syscalls returns.
   struct stat s;
   int retvalue;

   // Performing stat on file.
   retvalue = stat(path, &s);
   if (retvalue == -1) {
    // Error
    printf("Error in getting %s dictionary file informations.\n", path);
   }
   // Check if the file is regular.
   if(!S_ISREG(s.st_mode)){
        // Error not a regular file
        printf(" %s dictionary is not a regular file.\n", path);
    }

    // To store file content.
    char file[s.st_size];

    // Opening the file in readonly mode.
    int fd = open(path, O_RDONLY, NULL);
    if (fd == -1) {
        // Error
        printf("Error in opening %s dictionary file.\n", path);
    }

    // Reading the file content using a buffer of BUFFER_SIZE length.
    char buffer[BUFFER_SIZE];
    unsigned int counter = 0;
    while (1) {
        retvalue = read(fd, buffer, BUFFER_SIZE);
        if (retvalue == -1) {
            // Error
            printf("Error in reading %s dictionary file.\n", path);
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
                printf("Error in allocating heap space for a word of the dictionary file.\n");
            }
            c = 0;
            // Copying the word (char by char) in the allocated space if it is not '\n'.
            // The dictionary words are loaded with all characters UPPERCASE, regardless how
            // they are written in the file.
            for (unsigned int j = i - wl; j <= i; j++) 
                if (file[j] != '\n') words[counter - 1][c++] = toupper(file[j]);
            // Inserting string terminator.
            words[counter - 1][c] = '\0';
            wl = 0;
        }  else 
            wl++;
    }

    printf("Words dictionary succesfully loaded from %s file.\n", path);

}

// This function search a word in the current game matrix passed by char* word.
// It starts from a matrix element indicated by
// i and j, respectively the row and column indexes.
// It continues to search recursively on adjacent elements.
// It returns 1 if at least one occurrence of the word was found in the matrix, 0 otherwise.
int searchWordInMatrix(int i, int j, char* word) {

    // Validate matrix only at beginning.
    if (i == 0 && j == 0) validateMatrix();

    // Matrix access out of bounds.
    if (i < 0 || i > NROWS - 1 || j < 0 || j > NCOL - 1) return 0;
    char c = word[0];
    // End word reached, word found.
    if (c == '\0') return 1;
    // Checking if the current matrix element is equal to the next character of the word searched.
    if (matrix[i][j] == c) {
        // Handle Qu case.
        if (word[1] == 'U') word++;
        word++;
        return searchWordInMatrix(i - 1, j, word) + searchWordInMatrix(i, j - 1, word) + searchWordInMatrix(i, j + 1, word) + searchWordInMatrix(i + 1, j, word) > 0 ? 1 : 0;
    }
    return 0;

}

// This function validate a dictionary previous loaded with loadDictionary() function.
// If loadDictionary() hasn't be called before an error will be throw.
// Validate it means that a new global copy of "words" var is created and allocated on the heap.
// To know what is "words", refer to the function loadDictionary().
// This new copy will be called "words_copy".
// It will contains only the words present in "words" var AND in the current game matrix.
// So its use is optional just like loadDictionary() and will be useless if a dictionary
// is not set with the optional parameter --diz.
// To save memory, only the words_copy var will be allocated on the heap, its elements, 
// instead, char* (strings), will simply be copied from "words" var.
// Remember to call this function whenever the current game matrix is changed and a 
// dictionary has been set.
void validateDictionary(void) {

    // Dictionary not loaded.
    if (words == 0) {
        // Error
        printf("Error, cannot validate the words if a dictionary has not been previously loaded.\nCall loadDictionary() before and retry.\n");
    }

    // First validation. words_copy must be initialized.
    if (words_copy == 0) {
        words_copy = (char**) malloc(sizeof(char*) * words_len);
        if (words_copy == 0) {
            // Error
            printf("Error, cannot allocate heap memory for the words_copy.\n");
        }
        // Copyinig pointers from "words" global var.
        for (int i = 0; i < words_len; i++)
            words_copy[i] = words[i];
    }

    // Iterate on all words loaded in "words" var from the dictionary file.
    for (int x = 0; x < words_len; x++) {
        int found = 0;
        // Iterating on each character of the game matrix.
        for (int i = 0; i < NROWS; i++)
            for (int j = 0; j < NCOL; j++)
            // If at least one occurrence of the x-word of the loaded dictionary file
            // is found in the current game matrix, i will come out from these for.
                if((found = searchWordInMatrix(i, j, words[x])) && (i = NROWS) && (j = NCOL))
                ;
        // Deleting (INCREASING ONLY THE COPY OF "WORDS" POINTERS) the x-word not found.
        // At the end "words_copy" var will contain only all words from the previously loaded
        // dictionary file AND present in the current game matrix. 
        // The other words will be represented by '\0.
        if (!found)
            while(words_copy[x][0] != '\0') words_copy[x]++;

    }

    // Printing results.
    printf("Dictionary succesfully validated, founded in the current matrix, these words from dict file:\n");
    for (int i = 0; i < words_len; i++)
        if (words_copy[i][0] != '\0')
            printf("%s\n", words_copy[i]);

}

// This function validate a word sent by the client/player.
// It returns 0 if the word is not valid, 1 if it is.
// Takes as input a pointer to the word (string) to validate.
// If a dictionary is in use (--diz as been set),
// it searches the word in the dictionary of the words present in
// (through global var "char** words_copy", see validateDictionary()) the current game matrix,
// If no dictionary is in use, it only looks for the word in the current game matrix.
/*

    WARNING:
    By using a dictionary, as the size of the matrix, the length of the words, and the number
    of searches increase, the efficiency of the software improves because the more onerous
    operation of searching for a word in the matrix is performed only once when the current game 
    matrix changes. Otherwise, if the dictionary is not used,
    every time the user submits a word, the search has to be performed totally, nullifying
    previously performed searches.

*/
int validateWord(char* word) {

    // Validating the matrix.
    validateMatrix();

    // Dictionary not previous loaded.
    if (words_copy == NULL && words != NULL) {
        // Error
        printf("Error, validateWord() cannot continue if loadDictionary(...) has been called, but the validateDictionary() not.\n");
    }

    // Empty word.
    if (word == NULL) {
        // Error
        printf("Error, validateWord() received a NULL word.\n");
    }

    // Word length.
    size_t s = strlen(word);
    // If is set a constraint (WORD_LEN) on the word length, it is applied.
    if (WORD_LEN > 0 && s == WORD_LEN) return 0;
    // The client submitted word is totally converted
    // to the UPPERCASE version, format in which the characters in the game matrix, 
    // the words in the dictionary file and the alphabet used to generate random matrices,
    // are loaded into memory.
    // There will be no difference between a client input like "home"
    // or "HoMe" or "HOME".
    // Will all be accepted if the word is present in the dictionary file and the
    // current game matrix.
    char word_copy[s];
    for (int i = 0; i < s; i++)
        word_copy[i] = toupper(word[i]);
    word = word_copy;
    
    // NO DICTIONARY SET.
    if (words == NULL)
        return searchWordInMatrix(0, 0, word);

    // DICTIONARY HAS BEEN SET.
    // Searching the word in the dictionary with the current game matrix words.
    for (int i = 0; i < words_len; i++)
        if (words_copy[i][0] != '\0' && strcmp(words_copy[i], word) == 0) return 1;
    
    // If we arrive here, the word is not valid.
    return 0;

}

/*

##########################              EXAMPLE             ##########################

Since the previous functions might be unclear/complex let's see an example.

Let's assume that loadDictionary("/path/to/file.txt") has been called, now we will
have "char** words" global var, filled with all the words present in the dictionary
file "/path/to/file.txt" (assuming a word for line).

Being the content of /path/to/file.txt:
hello\n
dog\n
mum

Let's see the content of "char** words" after loadDictionary(...).
Let's assume a byte-addressable architecture, arrays are stored contiguously, sizeof(char) = 1 byte.
Allocated on heap.
char** words -> words[0] (char*) [starting h with = 0x16d22726c] -> "hello\0"
             -> words[1] (char*) [starting d with = 0x16d227272 (the previous + 6 bytes)] -> "dog\0"
             -> words[2] (char*) [starting m with = 0x16d227276 (the previous + 4 bytes)] -> "mum\0"

Being the current game matrix the following:
h e D F
O l l o
N M Z m
E T m u

The characters in the matrix are all uppercase.
In the notation of this example, I wrote some lowercase characters
to denote those that form the words of interest (present in the file).
Also, let's assume there is not the constraint of the fixed word length.
Also, let's assume the game is setted with CASE_SENSITIVE = 0, so it's case INSENSITIVE.

Now let's call validateDictionary(); At the end we will have "char** words_copy" global var
filled with:
char** words_copy -> words_copy[0] (char*) [0x16d22726c same pointer of words[0]] -> "hello\0"
                  -> words_copy[1] (char*) [0x16d227275 same pointer of words[1] + 3 bytes] -> "\0"
                  -> words_copy[2] (char*) [0x16d227276 same pointer of words[2]] -> "mum\0"

The word "dog" is not present because it is in the dictionary file, and so in the "char** words",
but NOT in the current game matrix. Instead "hello\0" and "mum\0" are present because were
founded BOTH in the dictionary file (char** words) and the current game matrix (look at
the lowercase letters above).

The "dog\0" word was not deleted, we simply updated the copied pointer, incrementing it.

Note that we are operating by exploiting the power of pointers, with their arithmetic,
without having two copies of the strings in memory, but only two arrays with their pointers (char*).

Now let's simulate a client action, it sends us "mum".
We call validateWord("mum");
We simply iterate on the "char** words_copy" by skipping the strings terminator.
When we found something different from the '\0' it means that word is present in the current
game matrix, and the "hard" part is finished, we will just check that it is identical to the
one received as input from the user and if so the word will be valid, invalid otherwise.

*/



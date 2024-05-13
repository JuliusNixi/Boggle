#include "data_structures.h"
#include "support_functions.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

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
    // validateDictionary()
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
                if (file[j] != '\n') words[counter - 1][c++] = toupper(file[j]);
            // Inserting string terminator.
            words[counter - 1][c] = '\0';
            wl = 0;
        }  else 
            wl++;
    }

    printf("Words dictionary succesfully loaded from %s file.\n", DICT_PATH);

}

// This function search, starting from an element of the current game matrix,
// rapresented by the inputs i and j (respectively row, column indexes)
// all the words present in the global var "char** words".
// It uses another global var "char** words_copy", and modifies this one.
// It updates every word in "char** words_copy" by removing the letter found 
// in the current game matrix from it.
// If all the letters of a word are founded at end, this word in the "char** words_copy"
// will be rapresented as '\0'.
void searchWordInMatrix(int i, int j) {

    // i and j are not valid. Trying to do an access out of bounds to the game matrix.
    // We must stop the recursive call.
    if (i < 0 || i > NROWS - 1 || j < 0 || j > NCOL - 1) return;

    // foundatleastone signals if at least one letter of a word is founded.
    // So if it positive, it means we should continue to check the adjacent
    // elements in the matrix, because it is possible that the word is present.
    // On the other hand, if it is 0, it means we must stop the search.
    int foundatleastone = 0;
    // Iterating on all words loaded with loadDictionary() from dictionary file
    // in the "char** words".
    for (int l = 0; l < words_len; l++) {
 
        // Current char element in the l-word to find.
        char c = words_copy[l][0];
        // Word already founded in the matrix, skipping to the next.
        if (c == '\0') continue;
        // Is present the current char element of the l-word in the matrix[i][j]?
        if (matrix[i][j] == c) {
            // Yes, at least one letter found.
            // Incrementing the pointer of the current l-word, in this way we will
            // continue to search for the next word char.
            (words_copy[l])++;
            foundatleastone++;
        } 
        
    }

    // Continuing the search on the adjacent elements.
    if (foundatleastone) {
        searchWordInMatrix(i - 1, j);
        searchWordInMatrix(i, j - 1);
        searchWordInMatrix(i, j + 1);
        searchWordInMatrix(i + 1, j);
    }

}

// The previous function searchWordInMatrix() modifies the "char** words_copy" var.
// At the end words_copy will contain, for each word of the dictionary file
// previous loaded (with loadDictionary()), only the remaining letters of
// the initial word that were not found in the current game matrix starting
// the search from an element of it.
// The words of which all letters have been found will be represented by '\0'.
// The function has two different behaviors, if it is the first time it is called
// (reported with words_copy[0] == NULL) it will just copy all words from "char** words"
// to "char** words_copy". If the function call, on the other hand, should be one after the 
// first, it will reset all words that have not been completely found, i.e.,
// those other than '\0'.
void restoreWords(void) {
    // First behavior.
    if (words_copy[0] == NULL) {
        for (int i = 0; i < words_len; i++)
            words_copy[i] = words[i];
        return;
    }
    // Second behavior.
    for (int i = 0; i < words_len; i++)
        if (words_copy[i][0] != '\0') words_copy[i] = words[i];
}

// This function validate a dictionary file previous loaded with loadDictionary().
// Validate means that "delete" all words loaded from the dictionary that are not
// present in the current game matrix.
// All the words present in the file are stored (with loadDictionary()) in the
// global "char** words" var.
// After this function execution, there will be another "char** words_copy" global
// var that will contain only the words present in the current game matrix (and
// obviously in the file, and so in the "char** words").
// WARNING: It MUST CALLED AFTER the function loadDictionary() to load
// the dictionary before.
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
    }

    // Setting the first char* in the array to NULL to signal the restoreWords()
    // function that is the first call of it.
    // The function has a different behavior the first time, compared with subsequent.
    words_copy[0] = NULL;
    restoreWords();

    // Iter on the letters in the current game matrix. 
    // For each of them I invoke an searchWordInMatrix()
    // function that will search for all the words contained in "char** words" (dictionary file),
    // and the restoreWords(), that will reset the words strings modified but
    // not completely founded ini the words_copy var.
    for (int i = 0; i < NROWS; i++)
        for (int j = 0; j < NCOL; j++) {
            searchWordInMatrix(i, j);
            restoreWords();
        }

    // Now words_copy should contain the words NOT present in the current game matrix,
    // instead, the words present will be reported as '\0'.
    // Now we will "invert" the dictionary, because we want the words present in the matrix.
    // In other words, we can say that we will calculate the complementary of the set.
    // At the end of this for, words_copy will contain only the words present in the 
    // current game matrix, instead, the words NOT present will be reported as '\0'.
    for (int i = 0; i < words_len; i++)
        if (words_copy[i][0] == '\0') words_copy[i] = words[i];
        else while ((words_copy[i][0])++ != '\0');

    printf("Dictionary succesfully validated, founded in the current matrix, these words from dict file:\n");
    for (int i = 0; i < words_len; i++)
        if (words_copy[i][0] != '\0')
            printf("%s\n", words_copy[i]);

}

// This function validate a word sent by the client/player.
// It returns 0 if the word is not valid, 1 if it is.
// Takes as input a pointer to the word (string) to validate.
// It searches the word in the dictionary of the words present in
// (through global var "char** words_copy") the current game matrix.
// WARNING: It MUST CALLED AFTER the function validateDictionary() to validate
// the dictionary before.
int validateWord(char* word) {

    if (words_copy == 0) {
        // Error
        printf("Error, cannot validate the word if a dictionary has not been previously validated.\nCall validateDictionary() before and retry.\n");
    }

    // If is set a constraint (WORD_LEN) on the word length, it is applied.
    if (WORD_LEN > 0 && strlen(word) == WORD_LEN) return 0;
    // If the CASE_SENSITIVE is 0 (false) the client word is totally converted
    // to the uppercase version, format in which the characters in the game matrix and 
    // the words in the dictionary file loaded into memory are also represented.
    // In this way there will be no difference between a client input like "home"
    // or "HoMe" or "HOME".
    // Will all be accepted if the word is present in the dictionary file and the
    // current game matrix.
    char word_copy[strlen(word)];
    if (!CASE_SENSITIVE) {
        for (int i = 0; i < strlen(word); i++)
            word_copy[i] = toupper(word[i]);
        word = word_copy;
    }

    // Searching the word in the dictionary.
    for (int i = 0; i < words_len; i++)
        if (words_copy[i][0] != '\0' && strcmp(words_copy[i], word) == 0) return 1;
    
    // If we arrive here, the word is not valid.
    return 0;

}

/*

##########################              EXAMPLE             ##########################

Since the previous functions might be unclear/complex let's see an example.

loadDictionary("/path/to/file.txt"); Now we will have "char** words" global var, filled with 
all the words present in the dictionary file "/path/to/file.txt" (assuming a word for line).

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

Be the current game matrix the following:
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



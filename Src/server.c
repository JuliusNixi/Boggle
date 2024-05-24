// Shared server files vars and libs.
#include "server.h"

// Current file vars and libs.
#include <math.h>

struct ClientNode* head = NULL; // Pointer to the client list head.
struct ClientNode* tail = NULL; // Pointer to the client list tail.

pthread_mutex_t listmutex = PTHREAD_MUTEX_INITIALIZER; // Mutex that will used from threads to manage interactions with the list of clients.

char** words = NULL;  // Pointer to a char[][] array that will be allocated on the heap. Each string rapresent a word/line on the dictionary file
char** words_copy = NULL;  // Copy of "words" to be used when the dictionary is validated, will contains only words present BOTH in the dictionary and the current game matrix.
unsigned int words_len = 0;   // Length of the BOTH char[][] above.

unsigned long int matchtime = 0;  // Time of the last game startup.
unsigned long int pausetime = 0;  // Time of the last pause startup.
#define PAUSE_DURATION 15 // Duration of the pause in seconds.

#define WORD_LEN 4  // If set to an integer greater than 0, the server will refuse all the words that not match this length.

int pauseon = 0;
pthread_mutex_t pausemutex = PTHREAD_MUTEX_INITIALIZER;













volatile sig_atomic_t signalsecurepause;
pthread_t pausethread;
int pausethreadend = 0;

struct Queue {
    unsigned long int points;
    int socket_client_fd;
    struct Queue* next;
};
struct Queue* headq;
struct Queue* tailq;

// SISTEMARE
unsigned long connectedplayers;
unsigned long activeplayers;
int sistemare = 0;


#define MAX_NUM_CLIENTS 32
unsigned int connectedclients;

void* scorer(void* args) {

    return NULL;
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

// This function fill the game matrix (global char[NROWS][NCOL]) of size as written in NCOL and NROWS
// by reading a file (with file path) received from CLI.
// By passing a valid path file as argument, the function will start to read matrices line
// by line (assuming one matrix for file line ended with \n) beginning from the first to the end of file.
// If NULL is passed as arg, the function will load the next
// matrix from the current file, otherwise will start from the first line of the new file passed.
// The current used file path is stored on the heap with a static char* var MAT_PATH.
// The matrix will be loaded with all UPPERCASE characters regardless of how the characters
// are written in the file.
// If the matrices read from the file run out, faced with a new function call with a NULL argument,
// the function will start over in a "circular" way from the first line of the same file.
void loadMatrixFromFile(char* path) {

    // char* file to store file content.
    // All the contents of the file is loaded into heap memory only the first time
    // so as to avoid costly I/O operations during the game,
    // for that the pointer and the stat need to be static.
    static char* file = NULL;
    // Stat to get file information.
    static struct stat s;

    // i is static, so i can remember between functions call
    // where I was left to read the buffer file.
    static unsigned int i = 0;

    // String that will be allocated on the heap and will rapresent the matrix file path
    // (if present in the CLI args).
    static char* MAT_PATH = 0;
    if (path != NULL) {
        // Releasing old path.
        if (MAT_PATH != NULL) free(MAT_PATH);
        // Allocating new memory for the new path.
        MAT_PATH = (char*) malloc(strlen(path) * sizeof(char));
        if (MAT_PATH == NULL) {
            // Error
            printf("Error in allocating heap memory for the matrices file path.\n");
        }
        // Copying the path to in the heap var.
        strcpy(MAT_PATH, path);
        // Resetting i, a new file is passed, so i must restart from line 1.
        i = 0;
        
        // IMPORTANT: to avoid loading same matrix forever in startGame().
        matpath = NULL;
    }else
        // First call with NULL path.
        if (MAT_PATH == NULL) {
            // Error
            printf("Error, loadMatrixFromFile() has not been initialized. Recall it with a valid file path.\n");
        }

    int retvalue;
    unsigned int counter = 0;


    // Loading file content first time.
   if (path != NULL) {

        // Performing stat on file.
        retvalue = stat(MAT_PATH, &s);
        if (retvalue == -1) {
                // Error
                printf("Error in getting %s matrices file information.\n", MAT_PATH);
        }
        // Check if the file is regular.
        if(!S_ISREG(s.st_mode)){
                printf("Error %s matrices file is not a regular file.\n", MAT_PATH);
                // Error
        }

        // Releasing previous file content if present.
        if (file != NULL)
            free(file);
        // Allocating heap memory for the new file content.
        file = (char*) malloc(sizeof(char) * s.st_size);
        if (file == NULL) {
            // Error
            printf("Error in allocating memory for the matrix file content.");
        }


        // Opening the file in readonly mode.
        int fd = open(MAT_PATH, O_RDONLY, NULL);
        if (fd == -1) {
            // Error
            printf("Error in opening %s matrices file.\n", MAT_PATH);
        }

        // Reading the file content using a buffer of BUFFER_SIZE length.
        char buffer[BUFFER_SIZE];
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
   }

    // Applying the next read matrix from file to the program matrix global var.
    // Initializing iterator.
    getMatrixNextIndexes(NULL);
    int matrixnextindexes[2];
    counter = 0;
    // If i equals to the file size, I am at the end of it and I start reading all over again.
    if (i == s.st_size) i = 0;
    for (; i < s.st_size; i++) {
        // Skipping spaces, and 'u' of 'Qu'.
        if (file[i] == ' ' || file[i] == 'u') continue;

        // New line found.
        if (file[i] == '\n') {
            // Getting matrix index to fill.
            getMatrixNextIndexes(matrixnextindexes);
            if (counter != NROWS * NCOL || matrixnextindexes[0] != -1) {
                // Error
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

// This function will be executed in a dedicated thread started as soon as possible in the main.
// It will run forever (as long as the process lives) looping in a while(1);.
// Will only deal with the management of SIGINT and SIGALRM signals, but
// all others could also be added without any problems.
// The SIGINT is sent when CTRL + C are pressed on the keyboard.
// The SIGALRM will be used to pause the game.
// All other threads will block these signals.
// This way should be better then registering the signals handlers with the sigaction().
// Because we won't be interrupted and thus solve the very annoying problem of using
// async-safe-signals functions (reentrant functions).
// Note that still the problem of inter-thread competition persists and needs to be handled.
void* signalsThread(void* args) {

    int sig;    
    int retvalue;
    while (1){
        // Intercepting signals previously setted in the mask in the main.
        // sigwait() atomically get the bit from the signals pending mask and set it to 0.
        retvalue = sigwait(&signal_mask, &sig);
        if (retvalue != 0) {
            // Error
            printf("Error in sigwait().\n");
        }

        // Treatment of different signals.
        switch (sig){
        case SIGINT:{ 

            break;
        }case SIGALRM:{
            // This will manage the SIGALRM signal triggered by the timer when the game is over.

            // The game is over.
            printf("\n\nGame FINISHED!\n\n");

            //mLock(&listmutex);
            
            pauseon = 1;

            // ... points total ...

            // Pause on.

            break;
        }default:
            // Error
            printf("");
            break;
        }
    }

    return NULL;

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

// This function check if the current matrix content (global char[][]) is legit
// in accordance with the alphabet.
// If is found in the matrix at least 1 character not present in the alphabet,
// the matrix is not valid, and a critical error is throw.
// If the matrix is valid, nothing happen.
// The alphabet is defined as a global #define in .h file.
// This is useful to immediately notice any developmental errors that could lead
// the matrix into an inconsistent state.
void validateMatrix(void) {

    for (unsigned int i = 0; i < NROWS; i++) {
        for (unsigned int j = 0; j < NCOL; j++){
            char c = matrix[i][j];
            unsigned int found = 0;
            // Searching char of matrix in the alphabet.
            // Matrix chars are saved in UPPERCASE.
            // Also the alphabet is always used in UPPERCASE.
            for (unsigned int x = 0; x < strlen(ALPHABET); x++)
                if (c == toupper(ALPHABET[x])) {
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

// This function initialize the game matrix (global server char[NROWS][NCOL]).
// Is not mandatory but recommended, to avoid annoying bugs during the development.
// The function will simply fill the matrix with a special undefined char (VOID_CHAR).
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

// This function allocate on the heap a string.
// It returns the pointer to it.
// It's length is calculated from NROWS and NCOLS data.
// The string will rapresent visually the game matrix.
// The Qu will be show in the result even if, only the character Q is stored in the game matrix.
char* serializeMatrixStr(void) {
/*
    
    Below i calculate the string length of the matrix.
    There are 4 addends enclosed in the parenthesis below.
    The first is the length of the only data contained in the matrix 
    multiplied by 2 because every position could be Qu.
    The second addend is the number of spaces betwen letters.
    The third rapresent the \n at the end of each line.
    The fourth plus 1 is the string terminator (\0).

    Is used to serialize the game matrix var into a visually pretty string.
    
*/
    // Matrix serialized string length.
    const unsigned int MAT_STR_LEN = (NCOL * NROWS * 2) + ((NCOL - 1) * NROWS) + (NROWS) + (1);

    // Allocating the string on the heap.
    char* matrixstring = (char*) malloc(sizeof(char)* MAT_STR_LEN);
    if (matrixstring == NULL) {
        // Error
        printf("Error in allocating heap memory for the matrix string.\n");
    }

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

    return matrixstring;

}


// This function allocate and load in memory on the heap a char array[][] called "words".
// "words" is a char** global var.
// Each line is a char* to a word (allocated on the heap) of the dictionary file.
// Assuming the dictionary file contains one word at each line terminated by \n.
// Dictionary file is used to check if a word submitted by a client is legit,
// in addition to the presence in the current game matrix.
// It used when the optional --diz is setted by CLI with a file path passed as arg.
// If --diz is not present, will be passed to this function DEFAULT_DICT #defined
// in the main server file.
// The function takes as input the file path to the dictionary.
// It set also a current file global var words_len, that rapresent the "words" length.
void loadDictionary(char* path) {

    // Dictionary already loaded, updating it.
    if (words != NULL) {
        // Cleaning words and words_copy.
        // Clear also words_copy is a good idea to not create a possible insubstantial state.
        for (int i = 0; i < words_len; i++)
            free(words[i]);
        free(words);
        words = NULL;
        words_len = 0;

        if (words_copy != NULL) {
            for (int i = 0; i < words_len; i++)
                free(words_copy[i]);  
            free(words_copy);
            words_copy = NULL;      
        }
    }

    // Empty path.
    if (path == NULL) {
        // Error
        printf("Error, loadDictionary() received an empty path.\n");
    }

   // Stat to get file information, retvalue to check syscalls returns.
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
        // Error
        printf(" %s dictionary is not a regular file.\n", path);
    }

    // To store file content.
    char file[s.st_size]; /* Total size, in bytes */

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
    for (unsigned int i = 0; i < s.st_size; i++)
        // End of line or end of file reached.
        if (file[i] == '\n' || i + 1 == s.st_size)
            counter++;
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
                printf("Error in allocating heap space for the %u word of the dictionary file.\n", counter);
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

    printf("Words dictionary succesfully loaded from %s file with %u words.\n", path, counter);

}

// This function validate a dictionary previous loaded with loadDictionary() function.
// If loadDictionary() hasn't be called before an error will be throw.
// Validate it means that a new global copy of "words" var is created and allocated on the heap.
// To know what is "words", refer to the function loadDictionary().
// This new copy will be called "words_copy", it will be (as "words") as char**.
// It will contains only the words present in "words" var AND in the current game matrix.
// To save memory, only the words_copy var will be allocated on the heap, its elements, 
// instead, char* (pointer to string), will simply be copied from "words" var.
// Then each char* of new "words_copy" will be modfied in this way:
// - if the "words_copy[i]" word is present in the current game matrix, the pointer 
// will remain the same, no modification is done.
// - otherwise, if the "words_copy[i]" word is NOT present in the current game matrix,
// the pointer (char*) will be increased to reach '\0'.
// In this way the dictionary file strings (words) will be present in memory only one time.
// Note that "words_copy" will also be useful as support to trace the player already submitted words.
// Since the "words_copy" is a copy of "words", "words_copy" length will be the same,
// already previously (with loadDictionary()) setted in the global current file var "words_len".
// IMPORTANT: Remember to call this function whenever the current game matrix is changed.
void validateDictionary(void) {

    // Dictionary not loaded.
    if (words == 0) {
        // Error
        printf("Error, cannot validate the words if a dictionary has not been previously loaded.\nCall loadDictionary() before and retry.\n");
    }

    // Realising (if already present) words_copy.
    if (words_copy != 0)
        free(words_copy);

    // Allocating space for the new words_copy.
    words_copy = (char**) malloc(sizeof(char*) * words_len);
    if (words_copy == 0) {
        // Error
        printf("Error, cannot allocate heap memory for the words_copy var.\n");
    }

    // Copyinig pointers from "words" current file global var.
    for (unsigned int i = 0; i < words_len; i++)
        words_copy[i] = words[i];

    // Iterate on all words loaded in "words" current file var from the dictionary file.
    for (unsigned int x = 0; x < words_len; x++) {
        int found = 0;
        // Iterating on each character of the game matrix.
        for (unsigned int i = 0; i < NROWS; i++)
            for (unsigned int j = 0; j < NCOL; j++)
            // If at least one occurrence of the x-word of the loaded dictionary file
            // is found in the current game matrix, I will come out from these for.
            // This is done thanks to the Short-circuit evaluation.
                if((found = searchWordInMatrix(i, j, words[x])) && (i = NROWS) && (j = NCOL))
                ;
        // Deleting (BY INCREASING ONLY THE COPY OF "WORDS" POINTERS) the x-word not found.
        // At the end "words_copy" var will contain only all words from the previously loaded
        // dictionary file AND present in the current game matrix. 
        // The other words will be represented by '\0'.
        if (!found)
            while(words_copy[x][0] != '\0') words_copy[x]++;
    }

    // Printing results.
    printf("Dictionary succesfully validated, founded in the current matrix, these words from dict file:\n");
    for (unsigned int i = 0; i < words_len; i++)
        if (words_copy[i][0] != '\0')
            printf("%s\n", words_copy[i]);

}

// This function can be used in two ways:
// - to simply check if a connected user is registered or not (by passing a NULL name as arg).
//   it returns 1 if registered, 0 otherwise.
// - to register a new user if the proposed username is not already registered.
//   it returns 1 if registered succesfully, 0 if the name is already present and so
//   no registration has been made. If successful it also initialize the struct NodeClient
//   name and words_validated fields.
int registerUser(char* name, struct ClientNode* user) {

    if (words == NULL || words_copy == NULL || words_len == 0) {
        // Error
        printf("Error, dictionary not loaded or validated. Call before loadDictionary() AND validateDictionary().\n");
    }

    // Invalid user arg check.
    if (user == NULL) {
        // Error
        printf("Invalid user in registerUser().\n");
    }

    // Only check if the user is registered without registering it when name == NULL is passed.
    if (name == NULL) {
        if (user->name != NULL) return 1;
        return 0;
    }

    // Normalizing in UPPERCASE the player name.
    toLowerOrUpperString(name, 'u');

    mLock(&listmutex);
    // Going through the list.
    struct ClientNode* current = head;
    struct ClientNode* prev = head;
    struct ClientNode* tmp = NULL;
    int found = 1;
    while (1) {
        if (current == NULL) break;
        // Already registered name.
        if (current->name != NULL && strcmp(current->name, name) == 0){
            found = 0;
            break;
        } 
        tmp = current;
        current = current->next;
        prev = current;
    }
    mULock(&listmutex);

    if (!found) return 0;

    // If we arrive here the user name is valid and not already registered.

    // Allocating heap space for the user username.
    char* str = (char*) malloc(sizeof(char) * strlen(name));
    if (str == NULL) {
        // Error
        printf("Error registering a new user.\n");
    }
    // Copying the new username in heap memory.
    strcpy(str, name);

    // Linking to the ClientNode relative field.
    user->name = str;

    // Now that the player is registered, i must allocate heap memory and copy
    // the "words_copy" global current file var, to trace
    // the already submitted words.
    user->words_validated = (char**) malloc(sizeof(char*) * words_len);
    if (user->words_validated == NULL) {
        // Error
        printf("Error in allocating heaap memory for words_validated of a client/player in registerUser().\n");
    }
    for (unsigned int i = 0; i < words_len; i++) (user->words_validated)[i] = words_copy[i];

    printf("Registered user with new name %s.\n", user->name);

    // Registered succesfully.
    return 1;

}

// This function simply set a POSIX timer using the global var gameduration, to handle the game time.
// The default gameduration is 3 minutes setted in main.
// The gameduration can also be inserted by a CLI arg.
void setAlarm(void) {

    // Alarm takes as input seconds, but the user input (gameduration var) is in minutes.
    // alarm(60 * duragamedurationtion);

    /*
    Time remaining to the next match: 1716168197 seconds.
    */


    gameduration = 15LU;
    alarm(gameduration);
    printf("The game duration is now setted to %lu minutes.\n", gameduration);

}

// This function start a new game.
void startGame(void) {

    printf("A new game is started right now.\n");

    //mLock(&pausemutex);

    // Getting new game start timestamp in POSIX.
    matchtime = (unsigned long) time(NULL);

    // Changing game matrix.
    if (usematrixfile)
        loadMatrixFromFile(matpath);
    else
        generateRandomMatrix();

    // Print the new current game matrix.
    char* mat = serializeMatrixStr();
    printf("Current new matrix:\n%s", mat);
    free(mat);

    // Validate the dictionary with the new game matrix.
    validateDictionary();

    // Setting the new pause timer.
    setAlarm();

}

// This function will run in a dedicated thread.
void* gamePause(void* args) {

    // Getting new starting pause timestamp in POSIX time.
    pausetime = (unsigned long) time(NULL);

    // Executing the pause.
    sleep(PAUSE_DURATION);

    // Ending pause.
    // ...
    
    // Starting a new game.
    startGame();

    // Terminating the thread. This WILL NOT EVEN return.
    pthread_exit(NULL);

}

// This function will wait a new client connection.
// It will accept it when present, will create a new client-list node on the heap.
// Will link the new node with the global current file list.
// Finally will start a new pthread to handle the new client connected.
// ClientNode is a global struct defined in .h file.
// This function will wait forever (as long as the a new connection happen).
// This function is called forever (as long as the process lives) in a main while(1);.
void acceptClient(void) {

    // Pre-allocating heap memory for a new client node.
    struct ClientNode* new = NULL;
    new = (struct ClientNode*) malloc(sizeof(struct ClientNode));
    if (new == NULL) {
        // Error
        printf("Error in allocating heap memory for a new player/client.\n");
    }

    // Waiting for a new client connection.
    int retvalue;
    new->client_address_len = (socklen_t) sizeof(new->client_addr);
    while (1) {
        new->socket_client_fd = accept(socket_server_fd, (struct sockaddr*) (&(new->client_addr)), &(new->client_address_len));
        if (new->socket_client_fd == -1) {
            // Error
            printf("Error in accepting a new client i will try again.\n");
        }else
            break;
    }
    printf("New client accepted.\n");

    // Initializing new client.
    new->next = NULL;
    // PTHREAD_MUTEX_INITIALIZER only available with statically allocated variables.
    // In this case i must use pthread_mutex_init().
    retvalue = pthread_mutex_init(&(new->handlerequest), NULL);
    if (retvalue != 0) {
        // Error
        printf("Error in mutex handle request initialization.\n");
    }
    new->points = 0U;
    // These below fields are truly initalized when the client is registered in registerUser().
    new->words_validated = NULL;
    new->name = NULL;

    // Adding the client to the list and updating global vars head and tail useful
    // to manage the list.
    // Lock the mutex.
    mLock(&listmutex);
    // Empty list.
    if (head == NULL) {
        head = new;
        tail = new;
    }else {
        // Not empty list.
        tail->next = new;
        tail = tail->next;
    }
    // Unlock the mutex.
    mULock(&listmutex);
    printf("New client added to the list.\n");
    
    // Starting a new pthread to handle the new client.
    // The executed function will be clientHandler().
    // It receives as input the pointer to the node (of its client to be managed)
    // of the clients list.
    retvalue = pthread_create(&(new->thread), NULL, clientHandler, new);
    if (retvalue != 0) {
        // Error
        printf("Error in starting a new client handler thread.\n");
    }
    printf("New client thread started.\n");

}

// This function is the "inverse" of atoi().
// It takes a number and returns a pointer to an heap allocated string containing the char 
// corresponding to the digits of the number received.
// It is used to convert values in string to send it in the char* data field of the struct Message.
// Specifically, for example, send the times.
char* itoa(uli n) {

    if ((int) n < 0 || n < 0) {
        // Error
        printf("Error itoa() received a negative number, must be positive.\n");
    }

    // Below i calculate the number of digits of the received n as input.
    // In this way i can allocate a string of the correct length without wasting space.
    // Based on StackOverflow, but tested and seems to work.
    uli ndigits = n == 0 ? 1LU : floor (log10 ( (n))) + 1LU;

    // Allocating heap space for the new string, of length calculated above.
    char* strint = (char*) malloc(sizeof(char) * ndigits);
    if (strint == NULL) {
        // Error
        printf("Error in casting integer to string, in itoa().\n");
    }

    // Inserting in the string the number received as input.
    sprintf(strint, "%lu", n);
    
    return strint;

}

// This function is a support to calculate the times needed.
uli timeCalculator(uli matchorpausetime, char mode) {

    mode = toupper(mode);

    if (mode != 'T' && mode != 'A') {
        // Error
        printf("Error in timeCalculator(), the second arg must be 'T' or 'A'.");
    }

    uli timenow = (uli) time(NULL);
    uli timeremaining;

    timeremaining = timenow - matchorpausetime;

    if (mode == 'T')
        // MSG_TEMPO_PARTITA 'T' Time to end game.
        timeremaining = gameduration - timeremaining;
    else
        // MSG_TEMPO_ATTESA 'A' Time remaining to the start of a new game, pause left time.
        timeremaining = (uli) PAUSE_DURATION - timeremaining;

    return timeremaining;

}

// This function simply serialize the current game matrix with serializeMatrixStr()
// and then send it (with sendMessage() function) to the received as input client.
void sendCurrentMatrix(struct ClientNode* client) {

    char* mat = serializeMatrixStr();
    sendMessage(client->socket_client_fd, MSG_MATRICE, mat);
    free(mat);
    printf("Matrix get request %s satisfied.\n", client->name);

}

// This function will run in a separate client dedicated thread and will manage the client requests.
// Each client is served by a thread.
// The returned object and the input are void*.
// Input must be converted to the correct type ClientNode*.
// It rapresent a pointer to the struct ClientNode of the client to be managed.
// The function will wait untill a message is received with read().
// Then it will process the request and will reply back to the client.
void* clientHandler(void* voidclient) {

    // Casting void* to ClientNode*.
    struct ClientNode* client = (struct ClientNode*) voidclient;

    struct Message* received = NULL;
    while (1) {

        // Waiting messages.
        received = receiveMessage(client->socket_client_fd);

        // Processing the new request.
        switch (received->type) {
            
            case MSG_MATRICE: {

                // Not logged in, r = 0.
                int r = registerUser(NULL, client);
                if (!r){
                    sendMessage(client->socket_client_fd, MSG_ERR, "You're not authenticated. Please sign up before.\n");
                    printf("A user requested the game matrix before the auth.\n");            
                    break;
                }

                // Pause game check.
                if (!pauseon) {
                    // Sending current game matrix.
                    sendCurrentMatrix(client);
                }else{
                    // The game is in pause.
                    // The reply must be:
                    // MSG_TEMPO_ATTESA 'A' Time remaining to the start of a new game, pause left time.

                    char* strint;
                    uli t;
                    
                    // Seconds left to the end of the pause.
                    t = timeCalculator(pausetime, MSG_TEMPO_ATTESA);
                    
                    // Casting the number to string to send it in the char* data of the struct Message.
                    strint = itoa(t);

                    // Sending MSG_TEMPO_ATTESA.
                    sendMessage(client->socket_client_fd, MSG_TEMPO_ATTESA, strint);
                    printf("Matrix get request from name %s converted in time request since the game is paused. Time to the next game: %lu.\n", client->name, t);
                    
                    free(strint);

                }

                break; 

            }case MSG_REGISTRA_UTENTE:{

                // Already logged in r = 1.
                int r = registerUser(NULL, client);
                if (r){
                    sendMessage(client->socket_client_fd, MSG_ERR, "You're already authenticated.\n");
                    printf("An user already registered, tried again to register, his name: %s.\n", client->name);
                    break;
                }

                // Trying to register the proposed name.
                r = registerUser(received->data, client);
                if (r){
                    // Registered succesfully.
                    sendMessage(client->socket_client_fd, MSG_OK, "Registered correctly with name.\n");
                    printf("User registered succesfully, request from name %s satisfied.\n", client->name);
                }else{
                    // Name already present.
                    sendMessage(client->socket_client_fd, MSG_ERR, "Name already present in the game, please chose a different one.\n");
                    printf("An user tried registering an already present name: %s.\n", received->data);
                    break;
                }

                // pauseon = 0 means the game is ongoing, must send the current game matrix.
                if (!pauseon)
                    sendCurrentMatrix(client);

                // MSG_TEMPO_ATTESA == MSG_TEMPO_RESTANTE (sul testo)
                // Sending MSG_TEMPO_ATTESA
                char* strint;
                uli t;
                if (pauseon) {
                    // The game is in pause.
                    // MSG_TEMPO_ATTESA 'A' Time remaining to the start of a new game, pause left time.
                    
                    // Seconds left to the end of the pause calculation.
                    t = timeCalculator(pausetime, MSG_TEMPO_ATTESA);
                    printf("Game in pause during new user signing up. Seconds left to the end of the pause (next game): %lu.\n", t);
                    
                    // Casting the number to string to send it in the char* data of the struct Message.
                    strint = itoa(t);
                    sendMessage(client->socket_client_fd, MSG_TEMPO_ATTESA, strint);

                }else{
                    // The game is ongoing.
                    // MSG_TEMPO_PARTITA 'T' Time to end game.

                    // Seconds left to the end of the game calculation.
                    t = timeCalculator(matchtime, MSG_TEMPO_PARTITA);
                    printf("Game going during new user signing up. Seconds left to the end of the game: %lu.\n", t);

                    // Casting the number to string to send it in the char* data of the struct Message.
                    strint = itoa(t);
                    sendMessage(client->socket_client_fd, MSG_TEMPO_PARTITA, strint);
                    
                }

                free(strint);
                break;

            }case MSG_PAROLA: {

                // Not logged in.
                int r = registerUser(NULL, client);
                if (!r) {
                    sendMessage(client->socket_client_fd, MSG_ERR, "You're not authenticated. Please sign up before.\n");
                    printf("An user tried to submit a word before the auth.\n");            
                    break;
                }

                // Game in pause.
                if (pauseon) {
                    sendMessage(client->socket_client_fd, MSG_ERR, "Cannot submit words during the game pause. Please, wait a new one.\n");
                    printf("User with name %s, submitted word \"%s\" during the game pause. We'll be ignored.\n", client->name, received->data);  
                    break;
                }

                // Submitting the word.
                int p = submitWord(client, received->data);


                if (p == -1) {
                    // -1, Invalid word. Not present in the matrix or WORD_LEN violated.
                    sendMessage(client->socket_client_fd, MSG_ERR, "Invalid word. Try again... Good luck!\n");
                    printf("User with name %s, submitted a IN-valid word \"%s\". Assigned 0 points, current total %u.\n", client->name, received->data, client->points);  
                    break;
                }

                char* strint = itoa((uli) p);
                if (p){
                    // Valid word, never submitted before, p are the achieved points for the word length.
                    sendMessage(client->socket_client_fd, MSG_PUNTI_PAROLA, strint);
                    printf("User with name %s, submitted a VALID, and never submitted before, word \"%s\". Assigned %d points, current total %u.\n", client->name, received->data, p, client->points);  
                }else{
                    // Already submitted p == 0.
                    sendMessage(client->socket_client_fd, MSG_PUNTI_PAROLA, strint);
                    printf("User with name %s, RE-submitted a valid word \"%s\". Assigned %lu points, current total %u.\n", client->name, received->data, (uli) p, client->points);  
                }

                free(strint);
                break;

            }case MSG_ERR : {

                break;

            }default:{

                // Not recognized message.
                break;

            } 

        } // Switch end.

        destroyMessage(&received);

    } // While end.

    return NULL;

}

// This function is used when a player submit a word.
// This function validate a word sent by the client/player.
// It takes as input a player struct pointer, and a char*
// rapresenting the word submitted to validate.
// It returns:
// - The points added to the player, if the word is correct (founded) and never submitted before.
// - 0, if the word is correct (founded) but ALREADY submitted before in the game.
// - -1, if the word is not correct (not founded) in the current game matrix.
int submitWord(struct ClientNode* player, char* word) {

    if (word == NULL || player == NULL) {
        // Error
        printf("Error in submitWord(), invalid player or word received.\n");
    }

    // Normalizing the word to UPPERCASE.
    toLowerOrUpperString(word, 'u');

    // i is the index of the searched word in the "words_copy" var.
    // "words_copy" mantain only the words present BOTH in the dictionary
    // AND in the current game matrix.
    // For more info see validateDictionary() function.
    int i = validateWord(word);

    // If -1 the word has not been found in the "words_copy" var.
    // This means that the searched word is not present in the current game matrix.
    // Note that it may still be in the dictionary, however.
    if (i == -1) return -1;

    // Below i use the i "words_copy" index to access to the "player->words_validated".
    // I can do this since are aligned on the words contained.

    // Already submitted word, 0 points.
    if (player->words_validated[i][0] == '\0') return 0;
    else {
        // Calculating the points to return.
        int p = (int) strlen(word);
        // Adding the points to the player's total.
        player->points += p;
        // 'Qu' correction, in strlen 'Q' and 'u' are counted as 2.
        // But we only want it to be worth 1 point.
        int qu = 0;
        for (unsigned int j = 0; j < p; j++)
            if (j + 1 != p && word[j] == 'Q' && word[j + 1] == 'U') qu++; 
        player->points -= qu;
        // Updating the char* pointer copy of the player
        // to remember that this word has already been submitted.
        while (player->words_validated[i][0] != '\0') (player->words_validated[i])++;
        // Returning the points.
        return p;
    }

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

// This function returns the index of the searched word in the dictionary 
// (and so the same as "words_copy").
// If the word is not valid (and so not present in the dict) it returns -1.
// Takes as input a pointer to the word (string) to search.
int validateWord(char* word) {

    // Validating the matrix.
    validateMatrix();

    // Dictionary not previous loaded.
    if (words == NULL) {
        // Error
        printf("Error, validateWord() cannot continue if loadDictionary() hasn't been called before.\n");
    }

    // Dictionary not previous validated.
    if (words_copy == NULL && words != NULL) {
        // Error
        printf("Error, validateWord() cannot continue if loadDictionary() has been called, but the validateDictionary() after not.\n");
    }

    // Empty word.
    if (word == NULL) {
        // Error
        printf("Error, validateWord() received an empty word.\n");
    }

    // Word length.
    int s = (int) strlen(word);
    // 'Qu' correction.
    for (unsigned int i = 0; i < WORD_LEN; i++)
        if (i + 1 != WORD_LEN && word[i] == 'Q' && word[i + 1] == 'U') s--;
    // If is set a constraint (WORD_LEN > 0) on the word length, it is applied.
    if (WORD_LEN > 0 && s < WORD_LEN) return -1;

    // The client submitted word is totally converted
    // to the UPPERCASE version, format in which the characters in the game matrix, 
    // the words in the dictionary file and the alphabet used to generate random matrices,
    // are loaded into memory.
    // There will be no difference between a client input like "home"
    // or "HoMe" or "HOME".
    // Will all be accepted if the word is present in the dictionary file and the
    // current game matrix.
    toLowerOrUpperString(word, 'u');

    // Searching the word in the dictionary containing only the the current game matrix words.
    for (unsigned int i = 0; i < words_len; i++)
        if (words_copy[i][0] != '\0' && strcmp(words_copy[i], word) == 0) return i;
    
    // If we arrive here, the word is not valid.
    return -1;

}

// This function is a cleanupper called before the exit of the program.
void clearExit(void){

    // TODO
    return;

}

/*

##########################              EXAMPLE             ##########################

Since the previous functions might be unclear let's see an example.

Let's assume that loadDictionary("/path/to/file.txt") has been called, now we will
have "char** words" var, filled with all the words present in the dictionary
file "/path/to/file.txt" (assuming a word for line terminated by \n).

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
Also, let's assume there is not the constraint of the word length.

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





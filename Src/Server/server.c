// ANCHOR File begin.
// Shared server cross files vars and libs.
#include "server.h"

// Current file vars and libs.
// Clients data.
struct ClientNode* head = NULL; // Pointer to the clients list head.
struct ClientNode* tail = NULL; // Pointer to the clients list tail.
pthread_mutex_t listmutex = PTHREAD_MUTEX_INITIALIZER; // Mutex that will used from threads to synchronize interactions with the list of clients above.
#define MAX_NUM_CLIENTS 0LU // This is the maximum number of connected clients to the server. It's use is optional since the clients data are stored in a unlimited (computational resources permitting) linked list. Set to 0 to disable, so to use no limit.
uli nclientsconnected = 0LU; // This rapresent the number of connected clients to the server. It's include BOTH registered and unregistered users.
#define NO_NAME "unregistered" // This will be the default name assigned to unregistered players.
uli clientid = 0LU;  // A temporary client's ID used to identify a client before its thread starting. After the thread starting the thread ID will be used an unique client identifier.

// Words data.
#define WORD_LEN 0LU  // If set to an integer greater than 0, the server will refuse all the words that not match this length, even if present in the dictionary and in the current game matrix. These words will be anyways showed as founded (if present in current game matrix and dictionary file) on the server output, but not validated when submitted by the players.
char** words = NULL;  // Pointer to a char[][] array that will be allocated on the heap. Each string rapresent a word/line of the dictionary file, NULL terminated.
char** words_valid = NULL;  // Copy of "words" above (char[][] array heap allocated) to be used when the dictionary is validated, will contains only the words present BOTH in the dictionary and the current game matrix. It will be updated every times that the current game matrix changes. The words present in the current game matrix will be copied (only the pointer, not the strings' data) from "words" above. Instead, the words not present in the current game matrix will be '\0'.
uli words_len = 0U;   // Length of BOTH char[][] above.

// Time data.
uli matchtime = 0LU;  // Time of the last game startup. Stored as seconds (UNIX time).
uli pausetime = 0LU;  // Time of the last pause startup. Stored as seconds (UNIX time).

#define MESSAGE_TIMEOUT_SECONDS 8LU // Amount of seconds after which the server close an unresponsive socket connection with a client. This is done to prevent that a client send only a part of the message, the server waits for the rest and the game stall for all for an undefined amount of time.

// Game/Pause synch data.
pthread_t gamepauseandnewgamethread; // This thread will execute the game pause and after will start a new game.
char pauseon = 0; // This var will be 1 if the game is paused, and 0 otherwise (game in play). It is used by the clientHandler() threads to understand how to response to the clients requests. This thread it's started at the end of the signalsThread() thread, before this last one come back to handle signals.
pthread_mutex_t pausemutex = PTHREAD_MUTEX_INITIALIZER; // This mutex will be used to synchronize threads during switching pause/game-on phases.
volatile sig_atomic_t* threadsignalreceivedglobal; // This is used by the clientHandler() thread, inside the SIGUSR1 signal handler, to write its client's receivedsignal without being able/needing to access the struct, to communicate with signalsThread().

// Shared queue data.
pthread_mutex_t queuemutex = PTHREAD_MUTEX_INITIALIZER; // This mutex will be used to synchronize threads ad the end of game to fill the shared queue.
struct Queue* headq = NULL; // Pointer to the queue head.
struct Queue* tailq = NULL; // Pointer to the queue tail.
uli nclientsqueuedone = 0LU; // This will store the number of clients that have already filled the queue.
char queuephasefinished = 0; // This var will be 1 if we are in pause and the queue filling phase has yet to be carried out, 0 otheriwse. It's used because a new client could connect during the sleeping pause (after the queue phase) and this must be handled to not cause troubles.

// Scorer thread data.
pthread_t scorert; // This thread will execute the scorer() function as described in the project statement. The thread is started by the signalsThread() thread.
char* scoreboardstr = NULL; // This will containt the final CSV sorted (by player's points descending) scoreboard heap allocated string. It will be sent at the end of game to all connected clients by the corresponding thread. It's created by the scorer() thread.

// ANCHOR scorer()
void* scorer(void* args) { // This thread is required by the assignment details project's document. This function will be executed in a dedicated thread. The thread is started by the signalsThread() thread. At end of game will pick up the content of the queue filled by all clients to produce the final game scoreboard, which will be sent by each client's thread to its managed user. This scoreboard will be a heap allocated string pointed by scoreboardstr (above global var). The scoreboard's format will be CSV (comma separeted values), sorted by player's points (descending). All clientHandler() threads will send this scoreboard to each player (BOTH registered or not). This thread will also determines the winner/s and will print it.

   // IT ASSUMES that needed mutexes are ALREADY LOCKED BY THE CALLER!

   fprintf(stdout, "I'm the scorer pthread (ID): %lu.\n", (uli) scorert);
   pthread_setname_np(scorert, "ScorerThread");

    // No players, nobody to send scoreboard... :(
    if (nclientsqueuedone == 0LU){
        fprintf(stdout, "No players, the scorer thread is useless... :(\n");
        scoreboardstr = NULL;
        pthread_exit(NULL);
    } 

    // Copying clients list in an array to use qsort().
    struct Queue* array[nclientsqueuedone];
    struct Queue* currentl = tailq;
    uli counter = 0LU;
    while (1) {
        if (currentl == NULL) break;
        array[counter++] = currentl;
        currentl = currentl->next;
    }

    // Sorting players by points using message's data.
    // To use qsort() i already copied the list in a temporary array.
     
    qsort(array, nclientsqueuedone, sizeof(struct Queue*), sortPlayersByPointsMessage);

    // Some prints.
    int retvalue = pthread_mutex_lock(&printmutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in scorer() thread, in locking printmutex.\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "\n\t\tFINAL SCOREBOARD\n");
    for (uli i = 0LU; i < nclientsqueuedone; i++) {
        char* s = serializeStrClient(array[i]->client);
        fprintf(stdout, "%s", s);
        free(s);
        s = NULL;
    }
    fprintf(stdout, "\n");
    retvalue = pthread_mutex_unlock(&printmutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in scorer() thread, in unlocking printmutex.\n");
        exit(EXIT_FAILURE);
    }

    // Taking from the previously sorted array the maximum number of points.
    char* pstr = csvNamePoints(array[0]->message, 1);
    uli p = (uli) strtoul(pstr, NULL, 10);
    free(pstr);
    pstr = NULL;

    // All players have 0 points.
    if (p == 0LU) {
        fprintf(stdout, "No winners, all players have 0 points.\n");
        // Continuing, i will send anyway the scoreboard.
    }

    // Counting the number of players that scored the same maximum points.
    counter = 0LU;
    uli cp;
    for (uli i = 0LU; i < nclientsqueuedone; i++) {
        // Getting points.
        pstr = csvNamePoints(array[i]->message, 1);
        cp = (uli) strtoul(pstr, NULL, 10);;
        free(pstr);
        pstr = NULL;
        // Counting the number of players that scored the same maximum points.
        if (p == cp) counter++;
        else break;
    }

    // Only one winner with maximum p points.
    if (p != 0LU && counter == 1LU) {
        char* n = csvNamePoints(array[0]->message, 0);
        fprintf(stdout, "The winner is %s. Scored %lu points.\n", n, p);
        free(n);
        n = NULL;
        // Continuing, i will send the scoreboard.
    }

    // More winners with the same amount of maximum points "p".
    retvalue = pthread_mutex_lock(&printmutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in scorer() thread, in locking printmutex at the end.\n");
        exit(EXIT_FAILURE);
    }
    if (p != 0LU && counter != 1LU) {
        fprintf(stdout, "The winners with %lu points are:\n", p);
        for (uli i = 0LU; i < nclientsqueuedone; i++) {
            // Getting current client's points.
            pstr = csvNamePoints(array[i]->message, 1);
            cp = (uli) strtoul(pstr, NULL, 10);
            free(pstr);
            pstr = NULL;
            // Printing multiple winners with same maximum "p" points scored.
            if (p == cp){
                char* n = csvNamePoints(array[i]->message, 0);
                fprintf(stdout, "%s\n", n);
                free(n);
                n = NULL;
            // The array is sorted. At the first different points score i could exit.
            }else break;
        }
    }
    retvalue = pthread_mutex_unlock(&printmutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in scorer() thread, in unlocking printmutex at the end.\n");
        exit(EXIT_FAILURE);
    }

    // Creating and printing the CSV final game scoreboard.
    // This is always created and delivered in the same way, regardless of the previous cases.
    // Is not sent only if there are no players.
    createScoreboard(array, nclientsqueuedone);

    fprintf(stdout, "\nHere the CSV scoreboard that will be sent to all clients:\n%s\n\n", scoreboardstr);
        
    pthread_exit(NULL);

}

// ANCHOR csvNamePoints()
// This function takes as input a struct Message* m.
// The message MUST contains in the "data" field a string in the format "playername,playerpoints"
// that will be tokenized.
// This function will return the player's name or the player's points by tokenizing the "data" field.
// The second function's arg is a char. It's' 1 if we want the "playername", 0 if we want
// the "playerpoints". In both cases the function will return a char* heap allocated string.
char* csvNamePoints(struct Message* m, char nameorpoints) {

    if (m == NULL) {
        // Error
        fprintf(stderr, "Error in csvNamePoints(), NULL message received.\n");
        exit(EXIT_FAILURE);
    }

    if (nameorpoints != 0 && nameorpoints != 1) {
        // Error
        fprintf(stderr, "Error in csvNamePoints(), nameorpoints must 0 or 1.\n");
        exit(EXIT_FAILURE);
    }

    // Copying message "data" field in a temporary string to use strtok().
    char* s = m->data;
    char backup[strlen(s) + 1]; // +1 for the '\0'.
    strcpy(backup, s);
    backup[strlen(s)] = '\0';

    // Getting name.
    char* tmp = strtok(backup, ",");
    uli namelen = strlen(tmp) + 1; // +1 for '\0'.
    char* name = (char*) malloc(sizeof(char) * namelen);
    if (name == NULL) {
        // Error
        fprintf(stderr, "Error in csvNamePoints(), malloc() of name failed.\n");
        exit(EXIT_FAILURE);
    }
    strcpy(name, tmp);
    name[strlen(tmp)] = '\0';

    // Getting points.
    tmp = strtok(NULL, ",");
    uli pointslen = strlen(tmp) + 1; // +1 for '\0'.
    char* points = (char*) malloc(sizeof(char) * pointslen);
    if (points == NULL) {
        // Error
        fprintf(stderr, "Error in csvNamePoints(), malloc() of points failed.\n");
        exit(EXIT_FAILURE);
    }
    strcpy(points, tmp);        
    points[strlen(tmp)] = '\0';

    // Choosing what return and freeing memory.
    char* ret;
    if (nameorpoints == 0) {
        // Wanted name.
        free(points);
        points = NULL;
        ret = name;
    }else{
        // Wanted points.
        free(name);
        name = NULL;
        ret = points;
    }

    return ret;

}

// ANCHOR sortPlayersByPointsMessage()
// This function is used in qsort() (in the scorer() thread) to sort an array of struct Queue*
// containing pointers to elements of the struct Queue.
// Each element contains a message with the "data" field in the format "playername,points".
int sortPlayersByPointsMessage(const void* a, const void* b) {

    struct Queue** x = (struct Queue**) a;
    struct Queue** y = (struct Queue**) b;

    struct Queue* xx = *x;
    struct Queue* yy = *y;

    struct Message* mx = xx->message;
    struct Message* my = yy->message;

    // Getting points from first object.
    char* s = csvNamePoints(mx, 1);
    uli px = (uli) strtoul(s, NULL, 10);
    free(s);
    s = NULL;

    // Getting points from second object.
    s = csvNamePoints(my, 1);
    uli py = (uli) strtoul(s, NULL, 10);
    free(s);
    s = NULL;

    return py - px;

}

// ANCHOR generateRandomMatrix()
// This function generate a random letters matrix of size as written in NCOL and NROWS.
// The letters that will be used are only those present in the ALPHABET.
// matrix, ALPHABET, NROWS and NCOLS are defined in the common.h.
// The matrix (global char[NROWS][NCOL]) will be filled with these letters.
void generateRandomMatrix(void) {

    int randint;
    // Iterating on game matrix.
    for (uli i = 0; i < NROWS; i++)
        for (uli j = 0; j < NCOL; j++) {
            // Choosing a random letter from ALPHABET and filling the matrix.
            randint = rand() % strlen(ALPHABET);
            // The characters written in the ALPHABET are used all in UPPERCASE version,
            // regardless how hey are written in the ALPHABET.
            matrix[i][j] = toupper(ALPHABET[randint]);
        }

    // Validating the new game matrix.
    validateMatrix();

    fprintf(stdout, "New random matrix created and validated succesfully.\n");

}

// ANCHOR loadMatrixFromFile()
// This function fills the game matrix (matrix is a global char[NROWS][NCOL])
// of size as written in NCOL and NROWS by reading a file (with file path) received as arg from CLI.
// matrix, NROWS and NCOLS are defined in the common.h.
// By passing a valid path file as argument, the function will start to read matrices line
// by line (assuming one matrix for file line ended with \n) beginning from the first to the 
// end of file. If NULL is passed as arg, the function will load the next
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
    // for that the pointer, the stat and the file descriptor need to be static.
    static char* file = NULL;
    // Stat to get file informations.
    static struct stat s;
    // Matrices file, file descriptor.
    static int fd = -1;

    // i will be the index used to read the file.
    // i is static, so i can remember between functions call
    // where I was left to read the file.
    static uli i = 0LU;

    // String that will be allocated on the heap and will rapresent the matrix file path
    // (if present in the CLI args).
    static char* MAT_PATH = NULL;
    if (path != NULL) {
        // Releasing old path if necessary.
        free(MAT_PATH);

        // Allocating new memory for the new path.
        MAT_PATH = (char*) malloc((strlen(path) + 1) * sizeof(char));
        if (MAT_PATH == NULL) {
            // Error
            fprintf(stderr, "Error in loadMatrixFromFile() in malloc() for the MAT_PATH.\n");
            exit(EXIT_FAILURE);
        }
        // Copying the path to in the heap var.
        strcpy(MAT_PATH, path);
        MAT_PATH[strlen(path)] = '\0';

        // Resetting i, a new file is passed, so i must restart from line 1.
        i = 0LU;
        
        // IMPORTANT: To avoid loading same matrix forever in startGame().
        matpath = NULL;
    }else{
        // First call with NULL path.
        if (MAT_PATH == NULL) {
            // Error
            fprintf(stderr, "Error in loadMatrixFromFile(), the function has been called for the first time with a NULL arg.\n");
            exit(EXIT_FAILURE);
        }
    }

    int retvalue;
    uli counter = 0LU;

    // Loading file content first time.
   if (path != NULL) {

        // Performing stat on file.
        retvalue = stat(MAT_PATH, &s);
        if (retvalue == -1) {
            // Error
            fprintf(stderr, "Error in loadMatrixFromFile() in stat().\n");
            exit(EXIT_FAILURE);
        }
        // Check if the file is regular.
        if(!S_ISREG(s.st_mode)){
            // Error
            fprintf(stderr, "Error in loadMatrixFromFile() the specified file is not regular.\n");
            exit(EXIT_FAILURE);
        }

        // Releasing previous file content if present.
        if (file != NULL) {
            free(file);
            file = NULL;
            retvalue = close(fd);
            if (retvalue == -1) {
                // Error
                fprintf(stderr, "Error in loadMatrixFromFile() in close() of the old file.\n");
                exit(EXIT_FAILURE);
            }else fd = -1;
        }
            
        // Allocating heap memory for the new file content.
        file = (char*) malloc(sizeof(char) * (s.st_size + 1)); // +1 for the '\0'.
        if (file == NULL) {
            // Error
            fprintf(stderr, "Error in loadMatrixFromFile() in malloc() for the file content.\n");
            exit(EXIT_FAILURE);
        }

        // Opening the file in readonly mode.
        fd = open(MAT_PATH, O_RDONLY, NULL);
        if (fd == -1) {
            // Error
            fprintf(stderr, "Error in loadMatrixFromFile() in open() of the file.\n");
            exit(EXIT_FAILURE);
        }

        // Reading the file content using a buffer of BUFFER_SIZE length.
        char buffer[BUFFER_SIZE];
        while (1) {
            retvalue = read(fd, buffer, BUFFER_SIZE);
            if (retvalue == -1) {
                // Error
                fprintf(stderr, "Error in loadMatrixFromFile() in read() of the file content.\n");
                exit(EXIT_FAILURE);
            }

            // Exit while, end of file reached.
            if (retvalue == 0) break;

            // Copying the buffer in the main file array.
            for (uli j = 0LU; j < retvalue; j++)
                file[counter++] = buffer[j];
        }

        file[counter] = '\0';

   }

    // Applying the next read matrix from file to the program matrix global var.

    // Initializing iterator.
    getMatrixNextIndexes(NULL);
    int matrixnextindexes[2];
    counter = 0LU;
    // If i equals to the file size, I am at the end of it and I start reading all over again.
    if (i == s.st_size) {
        i = 0LU;
        fprintf(stdout, "End of matrices file reached. Starting over again.\n");
    }
    // Reading and processing file content.
    for (; i < s.st_size; i++) {
        // Skipping spaces, '\r' and 'u' of 'Qu'.
        // https://stackoverflow.com/questions/1761051/difference-between-n-and-r
        if (file[i] == ' ' || file[i] == 'u' || file[i] == '\r') continue;

        // New line found or end of file reached.
        if (file[i] == '\n') {
            // Getting matrix index to fill.
            getMatrixNextIndexes(matrixnextindexes);
            if (counter != NROWS * NCOL || matrixnextindexes[0] != -1) {
                // Error
                // Check, i MUST to be at end of line and so end of matrix.
                // Invalid format file.
                fprintf(stderr, "Error in loadMatrixFromFile() the matrices file has an invaild format.\n");
                exit(EXIT_FAILURE);
            }
            i++;
            break;
        }

        // Getting next matrix indexes to write.
        getMatrixNextIndexes(matrixnextindexes);
        if (matrixnextindexes[0] == -1){
            // Error 
            // Invalid format file.
            fprintf(stderr, "Error in loadMatrixFromFile() the matrices file has an invaild format.\n");
            exit(EXIT_FAILURE);
        }else {
            // The matrix are loaded with all characters UPPERCASE, regardless how
            // they are written in the file.
            matrix[matrixnextindexes[0]][matrixnextindexes[1]] = toupper(file[i]);
            counter++;
        }

    }

    // Validating the new matrix.
    validateMatrix();

    fprintf(stdout, "New matrix succesfully loaded and validated from %s matrix file.\n", MAT_PATH);

}

// ANCHOR signalsThread()
// This function will be executed in a dedicated thread started as soon as possible in the main.
// It will run forever (as long as the process lives) looping in a while(1).
// Will only deal with the management of SIGINT, SIGALRM and SIGPIPE signals, but
// all others could also be added without any problems.
// The SIGINT is sent when CTRL + C are pressed on the keyboard.
// The SIGALRM will be used to trigger the game pause previously setted with alarm().
// The SIGPIPE will be simply ignored, because it's triggered when a client/server disconnection
// happens and this situation is already handled by the individual threads.
// All other threads will block these signals.
// This way should be better then registering the signals handlers with the sigaction().
// Because we won't be interrupted and thus solve the very annoying problem of using
// async-safe-signals functions (reentrant functions).
// Note that still the problem of inter-thread competition persists and needs to be handled.
void* signalsThread(void* args) {

    fprintf(stdout, "I'm the signalsThread() thread (ID): %lu.\n", (uli) signalsthread);

    int retvalue = pthread_mutex_lock(&setupmutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in signalsThread() thread, in locking setupmutex.\n");
        exit(EXIT_FAILURE);
    }
    setupfinished++;
    pthread_setname_np(signalsthread, "SignalsThread");
    retvalue = pthread_mutex_unlock(&setupmutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in signalsThread() thread, in unlocking setupmutex.\n");
        exit(EXIT_FAILURE);
    }

    int sig;    
    while (1){
        // Intercepting signals previously setted in the mask in the main.
        // sigwait() atomically get the bit from the signals pending mask and set it to 0.
        // https://stackoverflow.com/questions/2575106/posix-threads-and-signals
        retvalue = sigwait(&signalmask, &sig);
        if (retvalue != 0) {
            // Error
        }

        // Treatment of different signals.
        switch (sig){
            case SIGINT:{ 
                // SIGINT.
                fprintf(stdout, "\nCTRL + C: intercepted!\n");

                // Blocking all threads by acquiring all mutexes.
                int retvalue;
                retvalue = pthread_mutex_lock(&pausemutex);
                if (retvalue != 0) {
                    // Error
                    fprintf(stderr, "Error in signalsThread() thread, SIGINT, in locking pausemutex.\n");
                    exit(EXIT_FAILURE);
                }
                retvalue = pthread_mutex_lock(&listmutex);
                if (retvalue != 0) {
                    // Error
                    fprintf(stderr, "Error in signalsThread() thread, SIGINT, in locking listmutex.\n");
                    exit(EXIT_FAILURE);
                }
                retvalue = pthread_mutex_lock(&queuemutex);
                if (retvalue != 0) {
                    // Error
                    fprintf(stderr, "Error in signalsThread() thread, SIGINT, in locking queuemutex.\n");
                    exit(EXIT_FAILURE);
                }

                fprintf(stdout, "CTRL + C: Beginning exiting...\n");
                struct ClientNode* current = head;
                while (1) {
                    if (current == NULL) break;
                    retvalue = pthread_mutex_lock(&(current->handlerequest));
                    if (retvalue != 0) {
                        // Error
                        fprintf(stderr, "Error in signalsThread() thread, SIGINT, in locking handlerequest.\n");
                        exit(EXIT_FAILURE);
                    }            
                    current = current->next;
                }

                // Cancelling all the threads.
                retvalue = pthread_cancel(mainthread);
                if (retvalue != 0) {
                    // Error
                    fprintf(stderr, "Error in signalsThread() thread, SIGINT, in cancelling mainthread.\n");
                    exit(EXIT_FAILURE);
                }

                current = head;
                while (1) {
                    if (current == NULL) break;

                    // Notifying the clients.
                    sendMessage(current->socket_client_fd, MSG_ESCI, "Server is exiting...\n");

                    // Closing the client's socket connection.
                    retvalue = close(current->socket_client_fd);
                    if (retvalue == -1) {
                        // Error
                        fprintf(stderr, "Error in signalsThread() thread, SIGINT, in close() of a client's socket.\n");
                        exit(EXIT_FAILURE);
                    } 

                    // Cancelling the client's thread.
                    retvalue = pthread_cancel(current->thread);
                    if (retvalue != 0) {
                        // Error
                        fprintf(stderr, "Error in signalsThread() thread, SIGINT, in cancelling current->thread.\n");
                        exit(EXIT_FAILURE);
                    }

                    // Cleaning the client object.
                    struct ClientNode* tmp = current->next;
                    current->client_address_len = 0;
                    current->next = NULL;
                    current->points = 0LU;
                    free(current->words_validated);
                    current->words_validated = NULL;
                    free(current->name);
                    current->name = NULL;
                    free(current->registerafter);
                    current->registerafter = NULL;
                    current->actionstoexecute = 0;
                    current->receivedsignal = 0;
                    current->waiting = 0;
                    current->toexit = 0;
                    current->filledqueue = 0;
                    free(current);

                    current = tmp;
                }

                // Freeing words.
                for (uli i = 0LU; i < words_len; i++) {
                    free(words[i]);
                    words[i] = NULL;
                }
                free(words);
                free(words_valid);
                words = NULL;
                words_valid = NULL;

                // Clearing and destroying the queue.
                struct Queue* begin = tailq;
                struct Queue* tmp = NULL;
                while (1) {
                    if (begin == NULL) break;

                    tmp = begin->next;
                    struct Message* m = begin->message;
                    if (m != NULL) destroyMessage(&m);
                    begin->message = NULL;
                    free(begin);

                    begin = tmp;
                }

                // Closing server socket.
                retvalue = close(socket_server_fd);
                if (retvalue == -1) {
                    // Error
                    fprintf(stderr, "Error in signalsThread() thread, SIGINT, in close() of server socket.\n");
                    exit(EXIT_FAILURE);
                }

                fprintf(stdout, "%s\n", EXIT_STR);

                exit(EXIT_SUCCESS);
            }case SIGALRM:{
                // This will manage the SIGALRM signal triggered by the timer when the game is over.

                // ANCHOR Game Core.
                // This is the CORE of the server.

                /////////////////////////   SECURE CHECK   /////////////////////////
                // In this case the sync between threads to access to this var is not needed.
                if (pauseon) {
                    // Error
                    // Another pause is live. 
                    // Another unexpected SIGALRM received.
                    // Possible game/pause overlap.
                    fprintf(stderr, "CRITICAL DANGER ERROR: Overlapping pause!\n");
                    exit(EXIT_FAILURE);
                }

                /////////////////////////   GAME OVER   /////////////////////////
                // Some prints.
                retvalue = pthread_mutex_lock(&printmutex);
                if (retvalue != 0) {
                    // Error
                    fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in locking printmutex.\n");
                    exit(EXIT_FAILURE);
                }
                
                // Banner of closing previous "NEW GAME STARTED".
                char* banner = bannerCreator(BANNER_LENGTH, BANNER_NSPACES, "NEW GAME STARTED", BANNER_SYMBOL, 1);
                fprintf(stdout, "%s\n", banner);
                free(banner);
                banner = NULL;

                // Banner of "END GAME STARTED".
                banner = bannerCreator(BANNER_LENGTH, BANNER_NSPACES, "END GAME STARTED", BANNER_SYMBOL, 0);
                fprintf(stdout, "\n%s\n", banner);
                free(banner);
                banner = NULL;

                fprintf(stdout, "The game is just ended. The requests from clients received until now will anyways be completed.\n");
                
                fflush(stdout);

                retvalue = pthread_mutex_unlock(&printmutex);
                if (retvalue != 0) {
                    // Error
                    fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in unlocking printmutex.\n");
                    exit(EXIT_FAILURE);
                }

                struct ClientNode* current;










                //////////////////  QUEUE FILLING SETUP AND ENABLING PAUSE  //////////////////
                
                // Enabling pause and notifying the threads handlers of the end of game, to stop read()
                // and start working on queue.
                retvalue = pthread_mutex_lock(&pausemutex);
                if (retvalue != 0) {
                    // Error
                    fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in locking pausemutex.\n");
                    exit(EXIT_FAILURE);
                }
                retvalue = pthread_mutex_lock(&listmutex);
                if (retvalue != 0) {
                    // Error
                    fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in locking listmutex.\n");
                    exit(EXIT_FAILURE);
                }

                // Locking threads personal mutex.
                current = head;
                while (1) {
                    if (current == NULL) break;

                    // Locking threads handlers, when they complete their current request.
                    retvalue = pthread_mutex_lock(&(current->handlerequest));
                    if (retvalue != 0) {
                        // Error
                        fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in locking handlerequest.\n");
                        exit(EXIT_FAILURE);
                    }    

                    // Creating start game message to send it to the clients.
                    banner = bannerCreator(BANNER_LENGTH, BANNER_NSPACES, "START GAME", BANNER_SYMBOL, 1);              
                    uli l = strlen(banner) + 1 + 1; // +1 for the '\n'. +1 for the '\0'.
                    char msg[l];
                    sprintf(msg, "%s%c", banner, '\n');
                    msg[l - 1] = '\0';
                    free(banner);
                    banner = NULL;
                    sendMessage(current->socket_client_fd, MSG_OK, msg);

                    // Creating end game message to send it to the clients.
                    banner = bannerCreator(BANNER_LENGTH, BANNER_NSPACES, "END GAME", BANNER_SYMBOL, 0);
                    l = strlen(banner) + 1 + 1;
                    char finalmsg[l]; // +1 for the '\n'. +1 for the '\0'.
                    sprintf(finalmsg, "%s%c", banner, '\n');
                    finalmsg[l - 1] = '\0';
                    free(banner);
                    banner = NULL;
                    sendMessage(current->socket_client_fd, MSG_OK, finalmsg);

                    current = current->next;

                }

                // Enabling pause, it's safe because all the clients threads are SUSPENDED
                // on their mutexes or before on read() and cannot read pauseon.
                pauseon = 1;

                // Getting new starting pause timestamp in POSIX time.
                pausetime = (uli) time(NULL);

                // Starting queue phase.
                queuephasefinished = 0;

                current = head;
                // IMPORTANT: Reset actionstoexecute, receivedsignal, filledqueue and countertimeoutseconds.
                while (1) {
                    if (current == NULL) break;

                    current->actionstoexecute = 0;
                    current->receivedsignal = 0;
                    current->filledqueue = 0;
                    current->countertimeoutseconds = 0LU;

                    current = current->next;
                }

                current = head;
                // Notifying the threads handlers of end of game to stop read().
                while (1) {
                    if (current == NULL) break;

                    // Ping the client.
                    disconnecterChecker(&(current->socket_client_fd));

                    // Sending signal to each client thread to stop read().
                    threadsignalreceivedglobal = &(current->receivedsignal);
                    retvalue = pthread_kill(current->thread, SIGUSR1); 
                    if (retvalue != 0) {
                        // Error
                        fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in sending SIGUSR1.\n");
                        exit(EXIT_FAILURE);
                    } 

                    // Continuing only when the signal has been received by the client.
                    if (current->receivedsignal && current->waiting) current = current->next;
                    else{

                        // First time only, to not wait 1 second for each normal connected client.
                        if (current->countertimeoutseconds == 0) {
                            current->countertimeoutseconds++;
                            // Sleep 0.01 seconds.
                            usleep(10000);
                            continue;
                        }

                        // Ping.
                        disconnecterChecker(&(current->socket_client_fd));

                        // Kicking an unresponsive client to not block the game.
                        current->countertimeoutseconds++;
                        if (current->countertimeoutseconds == MESSAGE_TIMEOUT_SECONDS) {
                            // Disconnect the socket.
                            int retvalue = close(current->socket_client_fd);
                            if (retvalue == -1) {
                                // Error
                                fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in close() of an unresponsive client.\n");
                                exit(EXIT_FAILURE);
                            }else current->socket_client_fd = -1;
                        }

                        sleep(1);
                        
                        // Print.
                        if (MESSAGE_TIMEOUT_SECONDS - current->countertimeoutseconds <= 3) fprintf(stdout, "WAITING UNRESPONSIVE PLAYER (ID) %lu. KICKING HIM IN %lu SECONDS.\n", (uli) current->thread, MESSAGE_TIMEOUT_SECONDS - current->countertimeoutseconds);

                    } // End if.

                } // End while.

                // Now all clients threads (in clientHandler()) are suspended
                // on their mutexes (eventually read() are stopped),
                // except for those in disconnectClient(), but these last are not relevant.
                // Releasing all threads locks, now they can read the modified pauseon and will
                // work on the shared queue.
                current = head;
                while (1) {
                    if (current == NULL) break;

                    // Releasing client's personal mutex.
                    retvalue = pthread_mutex_unlock(&(current->handlerequest));
                    if (retvalue != 0) {
                        // Error
                        fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in unlocking handlerequest.\n");
                        exit(EXIT_FAILURE);
                    }

                    current = current->next;
                }
                // IMPORTANT: NOT realease listmutex.
                // Otherwise new clients might connect and get stuck in the receiveMessage().










                //////////////////  ENABLED PAUSE THREADS WORKING ON QUEUE  //////////////////

                fprintf(stdout, "Pause enabled. From now all the clients requests will be threated as in end game phase.\nClients released and notified, now they should working on the queue.\n");

                // STILL OWNING pausemutex -> registerUser() and disconnectClient() suspended.
                // STILL OWNING listmutex.

                //////////////////////////////////////////////////////////////////////////////
                // WARNING: NOW clientHandler() THREADS ARE RUNNING AGAIN AND CAN ACCESS TO //
                // struct ClientNode* OBJECT (their own mutex not entirely the list)!       //
                //////////////////////////////////////////////////////////////////////////////

                // REMEMBER: pauseon is still active and conditions the responses.

                // Threads working on queue...

                // Those blocked on registerUser() return to the beginning of clientHandler()
                // and can continue in this task, so there is no danger of deadlock.
                // Also for the disconnectClient() threads there is not the risk of deadlock.









                //////////////////  WAITING QUEUE  //////////////////

                // Checking if threads completed their jobs on queue.
                while (1) {
                    char toexit = 0;

                    retvalue = pthread_mutex_lock(&queuemutex);
                    if (retvalue != 0) {
                        // Error
                        fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in locking queuemutex.\n");
                        exit(EXIT_FAILURE);
                    }

                    uli filledqueueclients = 0LU;

                    // Counting how many clients are exiting and how many have already filled the queue.
                    current = head;
                    while(1) {
                        if (current == NULL) break;

                        if (current->filledqueue || current->toexit) filledqueueclients++;

                        current = current->next;
                    }

                    // All threads have succesfully filled the queue.
                    if (filledqueueclients == nclientsconnected) toexit = 1;

                    retvalue = pthread_mutex_unlock(&queuemutex);
                    if (retvalue != 0) {
                        // Error
                        fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in unlocking queuemutex.\n");
                        exit(EXIT_FAILURE);
                    }

                    // Sleep 0.01 seconds.
                    if (toexit) break;
                    else usleep(10000);

                }
                fprintf(stdout, "Queue has been succesfully filled by all the clients threads.\n");










                //////////////////  EXECUTING SCORER  //////////////////

                // Creating and executing the scorer thread.
                
                // STILL OWNING pausemutex -> registerUser() and disconnectClient() suspended.
                // STILL OWNING listmutex.
                retvalue = pthread_mutex_lock(&queuemutex);
                if (retvalue != 0) {
                    // Error
                    fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in locking queuemutex.\n");
                    exit(EXIT_FAILURE);
                }

                current = head;
                while (1) {
                    if (current == NULL) break;

                    // Locking threads handlers, when they complete their current request.
                    retvalue = pthread_mutex_lock(&(current->handlerequest));
                    if (retvalue != 0) {
                        // Error
                        fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in locking handlerequest.\n");
                        exit(EXIT_FAILURE);
                    }

                    current = current->next;
                }

                current = head;
                // IMPORTANT: Reset receivedsignals, waiting and countertimeoutseconds.
                while (1) {
                    if (current == NULL) break;

                    current->receivedsignal = 0;
                    current->waiting = 0;
                    current->countertimeoutseconds = 0LU;

                    current = current->next;
                }

                // Creating the scorer thread.
                retvalue = pthread_create(&scorert, NULL, scorer, NULL);
                if (retvalue != 0) {
                    // Error
                    fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in pthread_create() of scorer() thread.\n");
                    exit(EXIT_FAILURE);
                }
                // Waiting the score thread to finish.
                retvalue = pthread_join(scorert, NULL);
                if (retvalue != 0){
                    // Error
                    fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in pthread_join() of scorer() thread.\n");
                    exit(EXIT_FAILURE);
                }

                current = head;
                // Notifying the threads handlers of the completed thread scorer(), to send
                // the scoreboard to their clients.
                while (1) {
                    if (current == NULL) break;

                    // Ping the client.
                    disconnecterChecker(&(current->socket_client_fd));

                    // Sending signal to each client thread to stop read().
                    threadsignalreceivedglobal = &(current->receivedsignal);
                    retvalue = pthread_kill(current->thread, SIGUSR1); 
                    if (retvalue != 0) {
                        // Error
                        fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in sending SIGUSR1.\n");
                        exit(EXIT_FAILURE);
                    }

                    // Kicking an unresponsive client to not block the game.
                    if (current->receivedsignal && current->waiting == 1) current = current->next;
                    else {

                        // First time only, to not wait 1 second for each normal connected client.
                        if (current->countertimeoutseconds == 0) {
                            current->countertimeoutseconds++;
                            // Sleep 0.01 seconds.
                            usleep(10000);
                            continue;
                        }

                        // Ping.
                        disconnecterChecker(&(current->socket_client_fd));

                        // Kicking an unresponsive client to not block the game.
                        current->countertimeoutseconds++;
                        if (current->countertimeoutseconds == MESSAGE_TIMEOUT_SECONDS) {
                            // Disconnect the socket.
                            int retvalue = close(current->socket_client_fd);
                            if (retvalue == -1) {
                                // Error
                                fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in close() of an unresponsive client.\n");
                                exit(EXIT_FAILURE);
                            } else current->socket_client_fd = -1;
                        }

                        sleep(1);

                        // Print.
                        if (MESSAGE_TIMEOUT_SECONDS - current->countertimeoutseconds <= 3) fprintf(stdout, "WAITING UNRESPONSIVE PLAYER (ID) %lu. KICKING HIM IN %lu SECONDS.\n", (uli) current->thread, MESSAGE_TIMEOUT_SECONDS - current->countertimeoutseconds);
                    
                    } // End if.

                } // End while.

                // Now all clientHandler() thread are suspended on its mutex.
                // The clients in disconnectClient() not, but this is not a problem.

                // From 2 to 3, to adivse each clientHandler() thread that can send to
                // client the final CSV scoreboard.
                current = head;
                while (1) {
                    if (current == NULL) break;

                    current->actionstoexecute++;
                    
                    current = current->next;
                }

                current = head;
                while (1) {
                    if (current == NULL) break;

                    // Releasing client's personal mutex.
                    retvalue = pthread_mutex_unlock(&(current->handlerequest));
                    if (retvalue != 0) {
                        // Error
                        fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in unlocking handlerequest.\n");
                        exit(EXIT_FAILURE);
                    }

                    current = current->next;
                }

                retvalue = pthread_mutex_unlock(&queuemutex);
                if (retvalue != 0) {
                    // Error
                    fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in locking queuemutex.\n");
                    exit(EXIT_FAILURE);
                } 

                if (scoreboardstr != NULL) fprintf(stdout, "Scorer thread is terminated, the final CSV scoreboard should be filled.\n");
                else fprintf(stdout, "Scorer thread is terminated, since there are no players, there is no scoreboard... :(\n");









                //////////////////  CLIENTS END GAME WAITING COMUNICATION  //////////////////

                // Threads working on end game scoreboard send message.
                // After, threads can once again return to respond to clients' requests.

                // STILL OWNING pausemutex -> registerUser() and disconnectClient() suspended.
                // STILL OWNING listmutex.
                fprintf(stdout, "Clients released and notified, now they should communicate to their clients the CSV scoreboard.\n");
                while(1) {
                    char toexit = 0;

                    // Locking threads handlers, when they complete their current request.
                    current = head;
                    while (1) {
                        if (current == NULL) break;

                        retvalue = pthread_mutex_lock(&(current->handlerequest));
                        if (retvalue != 0) {
                            // Error
                            fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in locking handlerequest.\n");
                            exit(EXIT_FAILURE);
                        }

                        current = current->next;
                    }

                    current = head;
                    uli nclientsmessagesent = 0LU;
                    while(1) {
                        if (current == NULL) break;

                        if (current->actionstoexecute == 4 || current->toexit) {
                            // Client has sent the end game message or has exited.
                            nclientsmessagesent++;
                        }

                        current = current->next;
                    }

                    if (nclientsconnected == nclientsmessagesent) toexit = 1;

                    // Queue phase finished.
                    queuephasefinished = 1;

                    current = head;
                    while (1) {
                        if (current == NULL) break;

                        // Releasing client's personal mutex.
                        retvalue = pthread_mutex_unlock(&(current->handlerequest));
                        if (retvalue != 0) {
                            // Error
                            fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in unlocking handlerequest.\n");
                            exit(EXIT_FAILURE);
                        }

                        current = current->next;
                    }

                    // Sleep 0.01 seconds.
                    if (toexit) break;
                    else usleep(10000);

                }

                // Releasing scoreboardstr.
                free(scoreboardstr);
                scoreboardstr = NULL;

                // Releasing also the listmutex is important to avoid to blocks the
                // new user's socket connection acceptance during the game pause.
                retvalue = pthread_mutex_unlock(&listmutex);
                if (retvalue != 0) {
                    // Error
                    fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in unlocking listmutex.\n");
                    exit(EXIT_FAILURE);
                }
                // Releasing pausemutex, so registerUser() and disconnectClient() if
                // necessary could continue.
                retvalue = pthread_mutex_unlock(&pausemutex);
                if (retvalue != 0) {
                    // Error
                    fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in unlocking pausemutex.\n");
                    exit(EXIT_FAILURE);
                }











                //////////////////  CLEARING QUEUE  //////////////////

                // Clearing the queue, remember here the scorer thread is finished, due
                // to our pthread_join().
                // So locking this mutex now doesn't make sense, because no other thread
                // right now should be working on the shared queue, however it was done
                // for clarity.
                retvalue = pthread_mutex_lock(&queuemutex);
                if (retvalue != 0) {
                    // Error
                    fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in locking queuemutex.\n");
                    exit(EXIT_FAILURE);
                }

                // Freeing heap allocated memory.
                struct Queue* begin = tailq;
                struct Queue* tmp = NULL;
                while (1) {
                    if (begin == NULL) break;

                    tmp = begin->next;
                    struct Message* m = begin->message;
                    if (m != NULL) destroyMessage(&m);
                    begin->message = NULL;
                    free(begin);

                    begin = tmp;
                }

                // Resetting queue.
                nclientsqueuedone = 0LU;
                headq = NULL;
                tailq = NULL;
                fprintf(stdout, "Queue should be now cleared.\n");

                retvalue = pthread_mutex_unlock(&queuemutex);
                if (retvalue != 0) {
                    // Error
                    fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in unlocking queuemutex.\n");
                    exit(EXIT_FAILURE);
                }








                // Starting pause/new game thread.
                retvalue = pthread_create(&gamepauseandnewgamethread, NULL, gamePauseAndNewGame, NULL);
                if (retvalue != 0) {
                    // Error
                    fprintf(stderr, "Error in signalsThread() thread, SIGALRM, in pthread_create() of the gamePauseAndNewGame() thread.\n");
                    exit(EXIT_FAILURE);
                }

                // Back to handling signals.
                fprintf(stdout, "Signals thread back to handle signals.\n");

                break;

            }case SIGPIPE : {
                // https://stackoverflow.com/questions/108183/how-to-prevent-sigpipes-or-handle-them-properly
                // Nothing, already handled by the single threads.
                ; 
                break;
            } default: {
                // Error
                fprintf(stderr, "Error in signalsThread() thread, received an unexpected signal, its number %d.\n", sig);
                exit(EXIT_FAILURE);
            }
        } // End switch.

    } // End while.

    return NULL;

}

// ANCHOR getMatrixNextIndexes()
// This function is an iterator, useful to read/write the matrix in complex functions.
// It takes as input an integer array of 2 positions (pointer to it).
// It fills array[0] with the matrix current row index.
// It fills array[1] with the matrix current column index.
void getMatrixNextIndexes(int* matrixnextindexes) {

    // Static indexes, persitent between function calls.
    static uli i;
    static uli j;
    if (matrixnextindexes == NULL) {
        // Initialization or reset of the iterator is made by passing NULL as arg.
        i = 0LU;
        j = 0LU;
        return;
    }
    // Generating next indexes.
    for (; i < NROWS; ) {
        for (; j < NCOL; ) {
            matrixnextindexes[0] = i;
            matrixnextindexes[1] = j++;
            if (j != NCOL)
                return;
            j = 0LU;
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

// ANCHOR validateMatrix()
// This function checks if the current matrix content (global char[][] defined in common.h)
// is legit in accordance with the ALPHABET (also defined in common.h).
// If it's found in the matrix at least 1 character not present in the ALPHABET,
// the matrix is not valid, and a critical error is throw.
// If the matrix is valid, nothing happen.
// This is useful to immediately notice any developmental errors that could lead
// the matrix into an inconsistent state.
void validateMatrix(void) {

    for (uli i = 0LU; i < NROWS; i++) {
        for (uli j = 0LU; j < NCOL; j++){
            char c = matrix[i][j];
            char found = 0;
            // Searching char c of matrix in the alphabet.
            // Matrix chars are saved in UPPERCASE.
            // Also the ALPHABET is always used in UPPERCASE.
            for (uli x = 0LU; x < strlen(ALPHABET); x++)
                if (c == toupper(ALPHABET[x])) {
                    found = 1;
                    break;
                }
            // Character not found, error.
            if (found == 0) {
                // Error
                // Used this function also in "tests.c". When testing remove the exit to not stops the tests.
                fprintf(stderr, "Invalid matrix by validateMatrix()!\n");
                exit(EXIT_FAILURE);
            }
        }
    }

}

// ANCHOR serializeMatrixStr()
// This function allocates on the heap a string.
// It returns a pointer to it.
// The string's length is calculated from NROWS and NCOLS data.
// NCOLS and NROWS are defined in common.h.
// The string will rapresent visually the game matrix.
// The 'Qu' will be show in the result even if, only the character 'Q' is stored in the game matrix.
char* serializeMatrixStr(void) {
/*
    
    Below i calculate the string's length.

    There are 4 addends enclosed in the parenthesis below.
    The first is the length of the only data contained in the matrix 
    multiplied by 2 because every position could be Qu.
    The second addend is the number of spaces betwen letters.
    The third rapresent the '\n' at the end of each line.
    The fourth rapresent the '\0'.

    Is used to serialize the game matrix var into a string.
    Useful for sending or printing the game matrix quickly.
    
*/
    // Matrix serialized string length.
    const uli MAT_STR_LEN = (NCOL * NROWS * 2) + ((NCOL - 1) * NROWS) + (NROWS) + (1);

    /*
    Example of how a string matrix should be:
        Qu Qu Qu\n 
        Qu Qu Qu\n
        Qu Qu Qu\n\0
        3 x 3 matrix.
        3 * 3 = 9 Chars. 9 * 2 = 18 To handle 'Qu'.
        6 Spaces.
        3 '\n'.
        1 '\0'.
        18 + 6 + 3 + 1 = 28
        Indexes from 0 to 27. Where 27 is '\0'.
    */

    // Allocating the string on the heap.
    char* matrixstring = (char*) malloc(sizeof(char) * MAT_STR_LEN);
    if (matrixstring == NULL) {
        // Error
        fprintf(stderr, "Error in serializeMatrixStr(), bad malloc().\n");
        exit(EXIT_FAILURE);
    }

    // Checking the matrix validation.
    validateMatrix();

    // Initializing the string with a special character, useful for testing and debugging.
    for (uli i = 0; i < MAT_STR_LEN; i++)
        matrixstring[i] = VOID_CHAR;

    // Initializing iterator.
    getMatrixNextIndexes(NULL);
    int matrixnextindexes[2] = {0, 0};

    uli counter = 0LU;
    while (1) {

        getMatrixNextIndexes(matrixnextindexes);
        if (matrixnextindexes[0] == -1){
            // Inserting last '\n' and '\0'.
            matrixstring[counter] = '\n';
            // Insert string terminator.
            matrixstring[MAT_STR_LEN - 1] = '\0';
            break;
        } 

        // Inserting in the string a game matrix char.
        char c = matrix[matrixnextindexes[0]][matrixnextindexes[1]];
        matrixstring[counter] = c;

        // 'Qu' correction or space.
        counter++;
        if (c == 'Q') {
            matrixstring[counter] = 'u';
        }else{
            matrixstring[counter] = ' ';
        }

        // Inserting in the string a space (normal case) or a '\n' (end of line case).
        int j = matrixnextindexes[1];
        counter++;
        if (j == NCOL - 1) {
            // End line reached.
            matrixstring[counter] = '\n';
        }else{
            matrixstring[counter] = ' ';
        }

        counter++;

    }

    return matrixstring;

}

// ANCHOR loadDictionary()
// This function allocates and loads in memory on the heap a char array[][] called "words".
// "words" is a char** global var.
// Each line of the dictionary file is a char* to a word (allocated on the heap).
// Assuming the dictionary file contains one word at each line terminated by \n or \r\n.
// Dictionary file is used to check if a word submitted by a client is legit,
// in addition to the presence in the current game matrix.
// This function takes as input char* path, the dictionary file path to load.
// It set also a current file global var words_len, that rapresent the "words" length.
// All the words are loaded in memory in UPPERCASE, regardless how they are written in the file.
void loadDictionary(char* path) {

    // Empty path.
    if (path == NULL) {
        // Error
        fprintf(stderr, "Error in loadDictionary(), NULL path received.\n");
        exit(EXIT_FAILURE);
    }

    // Dictionary already loaded, updating it.
    if (words != NULL) {

        // Cleaning "words".
        for (uli i = 0LU; i < words_len; i++) {
            free(words[i]);
            words[i] = NULL;
        }
        free(words);
        words = NULL;
        words_len = 0LU;

        // Cleaning "words_valid".
        // Clearing also "words_valid" is a good idea to not create a possible insubstantial state.
        if (words_valid != NULL) {
            for (uli i = 0LU; i < words_len; i++) {
                free(words_valid[i]);  
                words_valid[i] = NULL;
            }
            free(words_valid);
            words_valid = NULL;      
        }

    }

    // Stat to get file information, retvalue to check syscalls returns.
    struct stat s;
    int retvalue;

    // Performing stat on file.
    retvalue = stat(path, &s);
    if (retvalue == -1) {
        // Error
        fprintf(stderr, "Error in loadDictionary(), in stat().\n");
        exit(EXIT_FAILURE);
    }
    
    // Check if the file is regular.
    if(!S_ISREG(s.st_mode)){
        // Error
        fprintf(stderr, "Error in loadDictionary(), the given file is not regular.\n");
        exit(EXIT_FAILURE);
    }

    // To store file content.
    // Total size, in bytes + 1 for the '\0'. 
    char file[s.st_size + 1];
    char file_copy[s.st_size + 1];

    // Opening the file in readonly mode.
    int fd = open(path, O_RDONLY, NULL);
    if (fd == -1) {
        // Error
        fprintf(stderr, "Error in loadDictionary(), in open().\n");
        exit(EXIT_FAILURE);
    }

    // Reading the file content using a buffer of BUFFER_SIZE length.
    char buffer[BUFFER_SIZE];
    uli counter = 0LU;
    while (1) {
        retvalue = read(fd, buffer, BUFFER_SIZE);
        if (retvalue == -1) {
            // Error
             fprintf(stderr, "Error in loadDictionary(), in read() of the file content.\n");
            exit(EXIT_FAILURE);
        }

        // Exit while, end of file reached.
        if (retvalue == 0) break;

        // Copying the buffer in the main file array.
        for (uli i = 0LU; i < retvalue; i++)
            file[counter++] = buffer[i];
    }
    // Terminating the file content.
    file[s.st_size] = '\0';

    // Copying the file content, to use strtok() on the first instance.
    strcpy(file_copy, file);
    file_copy[s.st_size] = '\0';

    // Counting file lines and allocating heap space.
    char* str = file;
    counter = 0LU;
    while (str != NULL) {
        // strtok() modifies the string content.
        // It tokenize all '\n' '\r' '\n\r' '\r\n'.
        // This tokens are replaced with '\0'.
        // The first time strtok() need to be called with string pointer, then with NULL.
        if (counter == 0LU) str = strtok(str, "\n\r");
        else str = strtok(NULL, "\n\r");
        counter++;
    }

    // Allocating heap spaces for "words" and setting "words_len".
    words_len = --counter;
    words = (char**) malloc(sizeof(char*) * words_len);
    if (words == NULL) {
        // Error
        fprintf(stderr, "Error in loadDictionary(), in malloc() of words.\n");
        exit(EXIT_FAILURE);
    }

    // Copying each word (line) of the file in "words[i]".
    counter = 0LU;
    str = file_copy;
    while (str != NULL) {
        // Totkenizing with strtok().
        if (counter == 0LU) str = strtok(str, "\n\r");
        else str = strtok(NULL, "\n\r");

        if (str == NULL) break;

        // Allocating heap space.
        words[counter++] = (char*) malloc(sizeof(char) * (strlen(str) + 1));
        if (words[counter - 1] == NULL) {
            // Error
            fprintf(stderr, "Error in loadDictionary(), in malloc() of a word.\n");
            exit(EXIT_FAILURE);
        }

        // Copying the word in the new words[i] heap space.
        strcpy(words[counter - 1], str);
        words[counter - 1][strlen(str)] = '\0';

    }

    // Converting all words to UPPERCASE.
    for (uli i = 0LU; i < words_len; i++)
        toLowerOrUpperString(words[i], 'U');

    fprintf(stdout, "Words dictionary succesfully loaded from %s file with %lu words.\n", path, counter);

}

// ANCHOR validateDictionary()
// This function validates a dictionary previous loaded in memory with loadDictionary() function.
// If loadDictionary() hasn't be called before an error will be throw.
// Validate means that a new global copy of "words" var is created and allocated on the heap.
// To know what is "words", refer to the function loadDictionary().
// This new copy will be called "words_valid", it will be (as "words") as char**.
// It will contain only the words present in "words" var AND in the current game matrix.
// To save memory, only the "words_valid" var will be allocated on the heap, its elements, 
// instead, char* (pointer to string), will simply be copied from "words" var.
// Then each char* of the new "words_valid" var will be modfied in this way:
// - if the "words_valid[i]" word is present in the current game matrix, the pointer 
// will remain the same, no modification is done.
// - otherwise, if the "words_valid[i]" word is NOT present in the current game matrix,
// the pointer (char*) will be increased to reach '\0'.
// In this way the dictionary file's strings (words) will be present in memory only one time.
// Since the "words_valid" is a copy of "words", "words_valid" length will be the same as "words",
// already previously (with loadDictionary()) setted in the global current file var "words_len".
// IMPORTANT: Remember to call this function whenever the current game matrix is changed.
void validateDictionary(void) {

    // Dictionary not loaded.
    if (words == NULL) {
        // Error
        fprintf(stderr, "Error in validateDictionary(), call loadDictionary() before.\n");
        exit(EXIT_FAILURE);
    }

    // Realising (if already present) "words_valid".
    free(words_valid);

    // Allocating space for the new words_valid.
    words_valid = (char**) malloc(sizeof(char*) * words_len);
    if (words_valid == NULL) {
        // Error
        fprintf(stderr, "Error in validateDictionary(), in malloc().\n");
        exit(EXIT_FAILURE);
    }

    // Copyinig pointers from "words" current file global var to the new "words_valid".
    for (uli i = 0LU; i < words_len; i++)
        words_valid[i] = words[i];

    // Iterate on all words loaded in "words" (current file global var) from the dictionary file.
    for (uli x = 0LU; x < words_len; x++) {
        char found = 0;

        // Iterating on each character of the game matrix.
        for (uli i = 0LU; i < NROWS; i++) {
            for (uli j = 0LU; j < NCOL; j++){
                // If the word is founded in the current game matrix, 1 is returned by searchWordInMatrix(),
                // 0, otherwise. When the word is founded the first time (found == 1),
                // we will set NROWS and NCOL to exit for prematurely.
                // If at least one occurrence of the x-word of the loaded dictionary file
                // is found in the current game matrix, I will come out from these for.
                // This is done thanks to the Short-circuit evaluation.
                if((found = searchWordInMatrix(i, j, words[x])) && (i = NROWS) && (j = NCOL)) 
                ;
            }
        }         

        // Deleting (BY INCREASING ONLY THE COPY FROM "WORDS" POINTERS) the x-word not found.
        // At the end "words_valid" var will contain only all words from the previously loaded
        // dictionary file AND present in the current game matrix. 
        // The other words will be represented by '\0'.
        if (!found) {
            while(words_valid[x][0] != '\0')
                words_valid[x] = words_valid[x] + 1;
        }

    }

    // Printing results.
    char found = 0;
    int retvalue;
    fprintf(stdout, "Dictionary succesfully validated, founded in the current matrix, these words from dict file:\n");
    
    // The founded words (valid words) in the dictionary file and now in the current game matrix,
    // will be written in a text file. This will be used to perform tests.
    int fileCurrentValidsWordsFD = open(VALID_WORDS_TESTS_FILE_PATH, O_TRUNC | O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);  
    if (fileCurrentValidsWordsFD == -1) {
        // Error
        fprintf(stderr, "Error in validateDictionary(), in open() of fileCurrentValidsWords.\n");
        exit(EXIT_FAILURE);
    }

    // Writing the word on the file and printing it.
    for (uli i = 0LU; i < words_len; i++) {
        // Valid word.
        if (words_valid[i][0] != '\0') {

            fprintf(stdout, "%s\n", words_valid[i]);

            // Writing the word on the file.
            retvalue = write(fileCurrentValidsWordsFD, words_valid[i], sizeof(char) * strlen(words_valid[i]));
            if (retvalue == -1) {
                // Error
                fprintf(stderr, "Error in validateDictionary(), in write() on fileCurrentValidsWords of a word.\n");
                exit(EXIT_FAILURE);
            }
            // Writing new line on the file.
            retvalue = write(fileCurrentValidsWordsFD, "\n", sizeof(char));
            if (retvalue == -1) {
                // Error
                fprintf(stderr, "Error in validateDictionary(), in write() on fileCurrentValidsWords of a new line.\n");
                exit(EXIT_FAILURE);
            }

            found = 1;

        }
    }
    
    // Closing the text file.
    retvalue = close(fileCurrentValidsWordsFD);
    if (retvalue == -1) {
        // Error
        fprintf(stderr, "Error in validateDictionary() in close() of the fileCurrentValidsWords file.\n");
        exit(EXIT_FAILURE);
    }

    // Printing some other stuff.
    if (found == 0) {
        fprintf(stdout, "No words of the current game matrix have been found in the dictionary file... :(\n");
        return;
    }
    fprintf(stdout, "The WORD_LEN value is: %lu.\n", WORD_LEN);
    if (WORD_LEN > 0LU)
        fprintf(stdout, "WORD_LEN greater than 0.\nEven if present in the list above, the words with a length < WORD_LEN will be refused.\n");
    else
        fprintf(stdout, "WORD_LEN is 0, all the above words will be accepted regardless their length.\n");

}

// ANCHOR registerUser()
// This function can be used in two ways:
// A. To simply check if a connected user is registered or not by passing a NULL "name" as arg.
//    It returns 1 if registered, 0 otherwise.
// B. To register a new user, in this case the returns code are:
//      - -1: An unexpected error happened.
//      - N: With N > 1, the proposed name contains a char that is againist the admitted chars ALPHABET.
//        N rapresent the code of the first not admitted char.
//      - -2: This is not an error. The registration process was interrupted by the end of the game.
//        The registration will be finished as soon as possible.
//      - -3: This is not an error. The proposed name is already registered in the game.
//      - -4: Completed registration succesfully.
//   If successful (-4), the "name" field of the struct ClientNode user will be filled
//   by setting it to a new char* heap allocated string, that will contain the new player registered name.
//   If successful (-4), this function also initializes the "words_validated" struct ClientNode
//   user field, by setting it to a new char** heap allocated strings array containing the copied 
//   pointers from "words_valid", but only the words that were not previously already accepted 
//   for the interested user for the current game will be different from '\0'.
//   When -2 is returned, it's possible a deadlock situation during the end of the game,
//   in that case, the function notify the deadlock, saving the message "m" in the user's struct
//   to process it later, and returns -2.
//   This function MUST be called already OWNING the handlerequest mutex. It will be up to the 
//   caller to release it after this function returns.
//   This function uses the pausemutex and the listmutex, but before returning always releases them.
int registerUser(char* name, struct ClientNode* user, struct Message* m) {

    // Invalid user arg check.
    if (user == NULL) {
        // Error
        fprintf(stderr, "Error in registerUser(), NULL user received.\n");
        return -1; 
    }

    if (words == NULL || words_valid == NULL) {
        // Error
        fprintf(stderr, "Error in registerUser(), call before loadDictionary() AND validateDictionary().\n");
        return -1;
    }

    // Only check if the user is registered without registering it when name == NULL is passed.
    if (name == NULL) {
        if (user->name != NULL) return 1;
        return 0;
    }

    // Normalizing in UPPERCASE the player name.
    toLowerOrUpperString(name, 'U');

    // Checking the conformity of the name against the ALPHABET.
    for (uli x = 0; x < strlen(name); x++) {
        char found = 0;
        for (uli i = 0; i < strlen(ALPHABET); i++) {
            char c = toupper(ALPHABET[i]);
            if (name[x] == c) {
                found = 1;
                break;
            }
        }
        if (found == 0) return name[x];
    }

    int retvalue;
    retvalue = pthread_mutex_trylock(&pausemutex);
    if (retvalue != 0) {
        if (retvalue == EBUSY) {
            // Pause is on!
            // DANGER OF DEADLOCK:
            // thread clientHandler() -> acquire lock client->handlerequest and calls this function
            // signalsThread() -> acquire listmutex lock list.
            // registerUser() (this function called by clientHandler()) -> wait on listmutex lock list.
            // signalsThread() -> wait on lock client->handlerequest.
            // Mutual waiting!
            // DEADLOCK!!!!
            // For this we have used pthread_mutex_trylock() and pausemutex here.

            // Saving the message to process it later.
            user->registerafter = m;

            // THIS IS NOT AN ERROR! IT'S NORMAL! REMEMBER TO RELEASE HANDLEREQUEST MUTEX
            // IN THE CALLER!
            // Pre-releasing the mutex to fix the deadlock.
            // Interrupted by end game.
            return -2;

        }else{
            // Error
            fprintf(stderr, "Error in registerUser(), in trylock() of pausemutex.\n");
            return -1;
        }
    }

    // TryLock success, owning pausemutex.

    // listmutex CANNOT BE acquired by the signalsThread() thanks to pausemutex, acquiring listmutex,
    // the acquisition of it before by other threads is not a problem because they always release it.

    retvalue = pthread_mutex_lock(&listmutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in registerUser(), in acquiring listmutex.\n");
        retvalue = pthread_mutex_unlock(&pausemutex);
        if (retvalue != 0) {
            // Error
            fprintf(stderr, "Error (DOUBLE) in registerUser(), in releasing pausemutex.\n");
            exit(EXIT_FAILURE);
        }   
        return -1;
    }


    // Owning listmutex.


    // If registerUser() is called, it means at least 1 player should be in the clients list.
    // Remember the clients list contains also the unregistered players, but currently connected.
    if (head == NULL || tail == NULL) {
        // Error
        fprintf(stderr, "Error in registerUser(), empty list.\n");
        exit(EXIT_FAILURE);
    }
    // Going through the list.
    struct ClientNode* current = head;
    char found = 0;
    while (1) {
        if (current == NULL) break;

        // Already registered name.
        if (current->name != NULL && strcmp(current->name, name) == 0){
            found = 1;
            break;
        } 

        current = current->next;
    }


    // Name already present.
    if (found) {
        // THIS IS NOT AN ERROR! IT'S NORMAL!
        retvalue = pthread_mutex_unlock(&listmutex);
        if (retvalue != 0) {
            // Error
            fprintf(stderr, "Error in registerUser(), unlocking listmutex.\n");
            return -1;
        }   
        retvalue = pthread_mutex_unlock(&pausemutex);
        if (retvalue != 0) {
            // Error
            fprintf(stderr, "Error in registerUser(), unlocking pausemutex.\n");
            return -1;
        }   
        return -3;
    } 

    // If we arrive here the user name is valid and not already registered.

    // Allocating heap space for the user username.
    char* str = (char*) malloc(sizeof(char) * (strlen(name) + 1)); // +1 for '\0'.
    if (str == NULL) {
        // Error
        fprintf(stderr, "Error in registerUser(), in malloc() of the username string.\n");
        retvalue = pthread_mutex_unlock(&listmutex);
        if (retvalue != 0) {
            // Error
            fprintf(stderr, "Error in registerUser(), unlocking listmutex.\n");
            return -1;
        }   
        retvalue = pthread_mutex_unlock(&pausemutex);
        if (retvalue != 0) {
            // Error
            fprintf(stderr, "Error in registerUser(), unlocking pausemutex.\n");
            return -1;
        }   
        return -1;
    }
    // Copying the new username in heap memory.
    strcpy(str, name);
    // Terminating string.
    str[strlen(name)] = '\0';

    // Linking to the ClientNode relative field.
    user->name = str;

    // Now that the player is registered, i must allocate heap memory and copy
    // the "words_valid" global current file var, to trace
    // the already submitted words.
    user->words_validated = (char**) malloc(sizeof(char*) * words_len);
    if (user->words_validated == NULL) {
        free(str);
        str = NULL;
        user->name = NULL;
        // Error
        fprintf(stderr, "Error in registerUser(), in malloc() of words_validated.\n");
        retvalue = pthread_mutex_unlock(&listmutex);
        if (retvalue != 0) {
            // Error
            fprintf(stderr, "Error in registerUser(), unlocking listmutex.\n");
            return -1;
        }   
        retvalue = pthread_mutex_unlock(&pausemutex);
        if (retvalue != 0) {
            // Error
            fprintf(stderr, "Error in registerUser(), unlocking pausemutex.\n");
            return -1;
        }   
        return -1;
    }
    // Copying pointers from words_valid to the new words_validated.
    for (uli i = 0; i < words_len; i++) (user->words_validated)[i] = words_valid[i];

    fprintf(stdout, "Registered user succesfully with the new name %s.\n", user->name);

    // Error
    retvalue = pthread_mutex_unlock(&listmutex);
    if (retvalue != 0) {
        // Error
        free(str);
        str = NULL;
        user->name = NULL;
        fprintf(stderr, "Error in registerUser(), unlocking listmutex.\n");
        return -1;
    }   
    retvalue = pthread_mutex_unlock(&pausemutex);
    if (retvalue != 0) {
        // Error
        free(str);
        str = NULL;
        user->name = NULL;
        fprintf(stderr, "Error in registerUser(), unlocking pausemutex.\n");
        return -1;
    }   

    // Registered succesfully.
    return -4;

}

// ANCHOR startGame()
// This function starts a new game.
// It saves the current time as start game time.
// It changes the current game matrix, by generating it or loading it from file.
void startGame(void) {

    // Printing banner.
    char* banner = bannerCreator(BANNER_LENGTH, BANNER_NSPACES, "NEW GAME STARTED", BANNER_SYMBOL, 0);
    fprintf(stdout, "\n%s\n", banner);
    free(banner);
    banner = NULL;
    fprintf(stdout, "A new game is started right now.\n");

    // Getting new game start timestamp time in POSIX.
    matchtime = (uli) time(NULL);

    // Changing game matrix.
    if (usematrixfile)
        loadMatrixFromFile(matpath);
    else
        generateRandomMatrix();

    // Print the new current game matrix.
    char* mat = serializeMatrixStr();
    fprintf(stdout, "New game matrix:\n%s", mat);
    free(mat);
    mat = NULL;

    // Validate the dictionary with the new game matrix.
    validateDictionary();

    // Setting the new pause timer.
    // The default gameduration is 3 minutes setted in main().
    // The gameduration can also be inserted by a CLI arg.
    alarm(gameduration);
    #if defined(TEST_MODE_SECONDS)
        fprintf(stdout, "The game duration timer is now setted to %lu SECONDS.\n", gameduration);
    #else 
        fprintf(stdout, "The game duration timer is now setted to %lu minutes.\n", gameduration / 60);
    #endif

}


// ANCHOR acceptClient()
// This function will wait a new client socket connection using accept() function.
// It will accept it when present, will create a new client-list node on the heap.
// Will link the new node with the global current clients list.
// Finally will start a new pthread to handle the new client connected.
// ClientNode is a global struct defined in server.h file.
// This function will wait forever (as long as the a new connection happen).
// This function is called forever (as long as the process lives) in a main while(1).
// This function will be executed always from the the main thread.
void acceptClient(void) {

    clientid++;

    // Waiting for a new client connection.
    int retvalue;
    int fd;
    struct ClientNode tmp;
    while (1) {
        fd = accept(socket_server_fd, (struct sockaddr*) (&(tmp.client_addr)), &(tmp.client_address_len));
        if (fd == -1) {
            // Error
            fprintf(stderr, "Error in accepting a new client. Trying to continue, so retrying...\n");
        }else
            break;
    }
    // Allocating heap memory for a new client node.
    struct ClientNode* new = NULL;
    new = (struct ClientNode*) malloc(sizeof(struct ClientNode));
    if (new == NULL) {
        // Error
        fprintf(stderr, "Error in acceptClient() in malloc().\n");
        exit(EXIT_FAILURE);
    }
    new->client_address_len = (socklen_t) sizeof(new->client_addr);
    new->socket_client_fd = fd;
    memcpy((void*)&(new->client_addr), (void*)&(tmp.client_addr), sizeof(tmp.client_addr));
    memcpy((void*)&(new->client_address_len), (void*)&(tmp.client_address_len), sizeof(tmp.client_address_len));
    fprintf(stdout, "New client succesfully accepted (TMP ID): %lu.\n", clientid);

    // Cannot use PTHREAD_MUTEX_INITIALIZER, because can be used only on static allocated mutexes.
    // Initialization should be performed like this.
    retvalue = pthread_mutex_init(&(new->handlerequest), NULL);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in acceptClient() in mutex init.\n");
        exit(EXIT_FAILURE);
    }

    // Initializing new client fileds.
    // int socket_client_fd; Previously setted.
    new->next = NULL;
    new->threadstarted = 0;
    new->points = 0LU;
    // These two below fields are truly initalized when the client is registered in registerUser().
    new->words_validated = NULL;
    new->name = NULL;
    /////////////////////
    new->registerafter = NULL;
    new->actionstoexecute = 0;
    new->receivedsignal = 0;
    new->waiting = 0;
    new->toexit = 0;
    new->filledqueue = 0;
    new->countertimeoutseconds = 0LU;

    // Adding the client to the list and updating global vars head and tail useful
    // to manage the list.
    // Lock the mutex.
    retvalue = pthread_mutex_lock(&listmutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in acceptClient() in locking listmutex.\n");
        exit(EXIT_FAILURE);
    }
    // Empty list.
    if (head == NULL || tail == NULL) {
        head = new;
        tail = new;
    }else {
        // Not empty list.
        tail->next = new;
        tail = tail->next;
    }
    fprintf(stdout, "New client succesfully added to the clients list (TMP ID): %lu.\n", clientid);
    
    nclientsconnected++;

    // Max clients (optional) feature. To disable set MAX_NUM_CLIENTS to 0.
    if (MAX_NUM_CLIENTS != 0 && MAX_NUM_CLIENTS == nclientsconnected) {
        retvalue = pthread_mutex_unlock(&listmutex);
        if (retvalue != 0) {
            // Error
            fprintf(stderr, "Error in acceptClient() in unlocking listmutex.\n");
            exit(EXIT_FAILURE);
        }
        sendMessage(new->socket_client_fd, MSG_ESCI, "Maximum number of clients reached. Disconnecting you... :(\n");
        disconnectClient(&new, 0);
        // IMPORTANT: Remember to not kill this thread! Otherwise no more acceptance of clients.
        return;    
    }

    // Starting a new pthread to handle the new client.
    // The executed function will be clientHandler().
    // The thread receives as input the pointer to the node (of its client to be managed)
    // of the clients list.
    // So, it won't have to cross through the list to operate with the client,
    // and that makes it a little easier to synchronize with others threads,
    // because for all those actions that act on the individual client (and do not affect
    // other clients) we can operate without the need for synchronization.
    retvalue = pthread_create(&(new->thread), NULL, clientHandler, new);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in acceptClient() in pthread_create().\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "New client thread started succesfully (TMP ID): %lu.\n", clientid);

    retvalue = pthread_mutex_unlock(&listmutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in acceptClient() in unlocking listmutex.\n");
        exit(EXIT_FAILURE);
    }
    
}

// ANCHOR timeCalculator()
// This function is used as support to calculate the times needed.
uli timeCalculator(uli matchorpausetime, char mode, char* returncode) {

    mode = toupper(mode);

    if (mode != 'T' && mode != 'A') {
        // Error
        fprintf(stderr, "Error in timeCalculator(), mode could be only 'T' or 'A'.\n");
        exit(EXIT_FAILURE);
    }

    uli timenow = (uli) time(NULL);
    uli timeremaining;

    timeremaining = timenow - matchorpausetime;
    uli tt;

    if (mode == 'T') {
        // MSG_TEMPO_PARTITA 'T' Time to end game.
        // returncode should be NULL here.
        tt = gameduration - timeremaining;
    }else {
        // returncode should be NOT NULL here.
        // MSG_TEMPO_ATTESA 'A' Time remaining to the start of a new game, pause left time.
        tt = ((uli) pauseduration) - timeremaining;
        if (timeremaining > (uli) pauseduration) {
            // Error
            fprintf(stderr, "WARNING: We are late! Processing is taking longer than the duration time of the pause.\nThe pause should have ended already.\nPlayers will be notified.\nConsider to increase pause duration time.\n");
            *returncode = 0;
        }else *returncode = 1;
    }

    return tt;

}

// ANCHOR clientHandler()
// This function will run in a separated dedicated thread and will manage a client's requests.
// Each client will be served by a thread.
// The returned object and the input are void*.
// The output will not be defined (not used, simply returning NULL).
// Input (received void*) must be converted to the correct type struct ClientNode*.
// It rapresents a pointer to the struct ClientNode of the client list to be managed.
// The function will wait untill a message is received with read() in receiveMessage().
// Then it will process the request and will reply back to the client.
// Each (this function) thread can be interrupted (during read() in receiveMessage()) 
// by the signal SIGUSR1 sent by the signalsThread() thread to notify of end game.
// After this, the clients threads will perform some end game actions and change how they respond
// to clients' requests noting that no game is in progress and we are in a pause.
// All requests (except registerUser() and disconnectClient()) that are running
// (and so received BEFORE) during the activation of the timer WILL BE COMPLETED.
// After that, the threads, will perform end game actions, specifically will fill a shared
// queue, will wait for the scorer() thread to makes the final CSV scoreboard and finally will send
// it to each corresponding client handled by the corresponding thread clientHandler().
void* clientHandler(void* voidclient) {

    if (voidclient == NULL) {
        // Error
        fprintf(stderr, "Error in clientHandler(), NULL client received.\n");
        exit(EXIT_FAILURE);
    }

    // Casting void* to ClientNode*.
    struct ClientNode* client = (struct ClientNode*) voidclient;

    struct Message* received = NULL;

    client->threadstarted = 1;

    int retvalue = pthread_mutex_lock(&printmutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in clientHandler(), in locking printmutex.\n");
        exit(EXIT_FAILURE);
    }

    // Setting thread name.
    char* thrid = itoa((uli) client->thread);
    pthread_setname_np(client->thread, thrid);
    free(thrid);
    thrid = NULL;

    fprintf(stdout, "CONNECTED: I'm a new clientHandler() thread (ID): %lu.\n", (uli) client->thread);
    // Printing the connected client's infos.
    char* strclient = serializeStrClient(client);
    fprintf(stdout, "CONNECTED: %s", strclient);
    free(strclient);
    strclient = NULL;

    retvalue = pthread_mutex_unlock(&printmutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in clientHandler(), in unlocking printmutex.\n");
        exit(EXIT_FAILURE);
    } 

    // Preliminary actions completed.

    while (1) {

        client->waiting = 0;
        char returncode;
        // This code could be:
        // - 0: A disconnection happened.
        // - 1: Nornally interrupted by a signal.
        // - 2: Unexpected error.
        // - 3: Read 0 bytes (at the message beginning, so the message's type).
        // - 4: Completed message received succesfully.
        if (client->registerafter == NULL) {
            // Waiting a message.
            received = receiveMessage(client->socket_client_fd, &returncode);
        }else{
            // There is a previous registerUser() message to be processed because of an interruption
            // due to the end of the game.
            // To remember that the message is still in process and the received content is valid.
            received = client->registerafter;
            client->registerafter = NULL;

            returncode = 4;
        }
// **********     MARK 1     **********

        // EVERY REQUEST RECEIVED BEFORE the TIMER, will be processed
        // EXCEPT the registerUser() and disconnectClient(), that will still be 
        // executed automatically after the management of the end of game.
        /*
        
        man pthread_mutex_lock

        [...] If a signal is delivered to a thread waiting for a mutex, upon return from 
        the signal handler the thread shall resume waiting for the mutex as if it was
        not interrupted. [...]
        
        */
        
 // **********     MARK 2     **********

        // Acquiring the lock.
        while (1) {
            // Trylock needed.
            retvalue = pthread_mutex_trylock(&(client->handlerequest));
            if (retvalue != 0) {
                if (retvalue == EBUSY) {
                    client->waiting = 1;
                    sleep(1);
                    continue;
                }else{
                    // Error
                    fprintf(stderr, "Error in clientHandler(), in trylock.\n");
                    exit(EXIT_FAILURE);
                }
            }
            // Lock acquired.
            break;
        }


        // Processing the request.

        char sleepflag = 0;

        // Probably disconnect.
        if (returncode == 0) {
            retvalue = pthread_mutex_unlock(&(client->handlerequest));
            if (retvalue != 0) {
                // Error
                fprintf(stderr, "Error in clientHandler(), in unlocking handlerequest.\n");
                exit(EXIT_FAILURE);
            }
            // Message already destroyed.
            disconnectClient(&client, 1);
            // Now this thread should be died.
        }

        // Unexpected error.
        if (returncode == 2) {
            // Error
            retvalue = pthread_mutex_unlock(&(client->handlerequest));
            if (retvalue != 0) {
                // Error
                fprintf(stderr, "Error in clientHandler(), in unlocking handlerequest.\n");
                exit(EXIT_FAILURE);
            }

            // Message already destroyed.

            fprintf(stderr, "Error in clientHandler(), unexpected error in the received message.\n");
            exit(EXIT_FAILURE);
        }

        // Nothing received in both cases, interrupted waiting message's type.
        // Message already destroyed in both cases.

        // Nothing to do, normally interrupted by a signal.
        if (returncode == 1)
        ;
        // Nothing to do, interrupted but readed 0.
        if (returncode == 3)
        ;

        // Here returncode == 4.

        // If a player no sends requests an entire game and an entire pause,
        // in the next new game will still have client->actionstoexecute > 0 (if no resetted)!
        // So, it will not fill the queue and the signalsThread() thread
        // will wait forever -> DEADLOCK!
        // For that is important to reset client->actionstoexecute in the signalThread() thread.
        
        // Executed when the game is paused.
        if (pauseon)
            // First time after end game and client NOT connected after scoreboard sent during the pause.
            if (client->actionstoexecute == 0 && queuephasefinished == 0){
                
                // The below case was made to handle this situation:
                // We have received a message and the game end timer rang
                // BETWEEN the reception of the message and the acquisition of the personal mutex.
                // Specifically between MARK 1 and MARK 2 (look at the code above).

                // Moreover, the signalsThread() thread blocks all the clientHandler() threads,
                // by acquiring all their personal mutexes, enables the pause and releases
                // all the previous acquired mutexes, but in the "received" var there is
                // a message received before.

                if (received != NULL) {
                    sleepflag = processReceivedRequest(&received, client, 1);
                    if (received != NULL) destroyMessage(&received);
                }else{
                    // Received NULL, no messages to process, go ahead to client->actionstoexecute == 1.
                    ;
                }

                client->actionstoexecute++;

            } 

        // Sending end of game message on queue.
        if (client->actionstoexecute == 1) {
            gameEndQueue(client);
            client->actionstoexecute++;
        }

        // client->actionstoexecute == 2 not present here because is the signalsThread() thread
        // that must execute client->actionstoexecute++,
        // to notify these threads of the end of scorer() thread and so that they can send to
        // their clients the final CSV scoreboard.

        // Sending the final scoreboard to the client.
        if (client->actionstoexecute == 3) {
            sendMessage(client->socket_client_fd, MSG_PUNTI_FINALI, scoreboardstr);
            client->actionstoexecute++;
        }

        // Processing the received client's request.
        sleepflag = sleepflag || processReceivedRequest(&received, client, 0);

        // This NULL check is mandatory because the message may have already been destroyed
        // with the previous code.
        if (received != NULL) destroyMessage(&received);

        retvalue = pthread_mutex_unlock(&(client->handlerequest));
        if (retvalue != 0) {
            // Error
            fprintf(stderr, "Error in clientHandler(), in unlocking handlerequest.\n");
            exit(EXIT_FAILURE);
        }

        // Sleep 0.1 seconds.
        if (sleepflag) usleep(100000);

    } // While end.

    return NULL;

}

// ANCHOR submitWord()
// This function is used when a player submits a word.
// This function validates a word sent by the client/player.
// It takes as input a player struct ClientNode* pointer that is the submitter, and a char* word
// rapresenting the word submitted to validate, NULL terminated.
// It returns:
// - The points added to the player, if the word is correct (founded both in dictionary file 
// and in the current game matrix) and never submitted before.
// - 0, if the word is correct (founded both in dictionary file and in the current game 
// matrix) but ALREADY submitted (from the interested player) before in the current game.
// - -1, if the word is not correct i.e. not founded in the current game matrix, but
// could be present in the dictionary.
int submitWord(struct ClientNode* player, char* word) {

    if (word == NULL || player == NULL) {
        // Error
        fprintf(stderr, "Error in submitWord(), NULL word or player.\n");
        exit(EXIT_FAILURE);
    }

    // Normalizing the word to UPPERCASE.
    toLowerOrUpperString(word, 'U');

    // i is the index of the searched word in the "words_valid" var.
    // "words_valid" mantain only the words present BOTH in the dictionary
    // AND in the current game matrix.
    // For more info see validateDictionary() function.
    int i = validateWord(word);

    // If i == -1 the word has not been found in the "words_valid" var.
    // This means that the searched word is not present in the current game matrix.
    // Note that it may still be in the whole dictionary (and so "words" var), however.
    if (i == -1) return -1;

    // Below I use the i "words_valid" index to access to the "player->words_validated".
    // I can do this since "words", "words_valid" and "words_validated" are all aligned
    // on the words contained.

    // Already submitted word, 0 points.
    if (player->words_validated[i][0] == '\0') return 0;
    else {
        // Calculating the points to return.
        uli p = strlen(word);
        // 'Qu' correction, in strlen() 'Q' and 'u' are counted as 2,
        // but we only want it to be worth 1 point.
        uli qu = 0LU;
        for (uli j = 0LU; j < p; j++)
            if (j + 1 != p && word[j] == 'Q' && word[j + 1] == 'U') qu++; 
        p -= qu;
        // Adding the points to the player's total.
        player->points += p;
        // Updating the char* pointer copy, of the player "words_validated",
        // to remember that NOW this word has already been submitted.
        while (player->words_validated[i][0] != '\0') (player->words_validated[i])++;
        // Returning the points.
        return (int) p;
    }

}

// ANCHOR searchWordInMatrix()
// This function searches a word in the current game matrix, passed by char* word.
// It starts from a matrix element indicated by
// i and j, respectively the row and column indexes.
// It continues to search recursively on adjacent elements (but not the diagonal ones).
// A letter of the matrix CANNOT BE USED MULTIPLE times to compose the word.
// It returns 1 if at least one occurrence of the word was found in the matrix, 0 otherwise.
char searchWordInMatrix(uli i, uli j, char* word) {

    // Matrix access out of bounds.
    if (i < 0 || i > NROWS - 1 || j < 0 || j > NCOL - 1) return 0;

    char c = word[0];

    // End word reached, word found.
    if (c == '\0') return 1;

    // Checking if the current matrix element is equal to the next (current) character of the word
    // searched.
    if (matrix[i][j] == c) {
        // Handle 'Qu' case.
        if (c == 'Q' && word[1] == 'U') word++;

        word++;

        // Temporarily marking the character as VOID_CHAR, to indicate that it has already
        // been used in subsequent function calls.
        matrix[i][j] = VOID_CHAR;

        char s = searchWordInMatrix(i - 1, j, word) + searchWordInMatrix(i, j - 1, word) + searchWordInMatrix(i, j + 1, word) + searchWordInMatrix(i + 1, j, word) > 0 ? 1 : 0;
        
        // Restoring the original character in the matrix and returning the result.
        matrix[i][j] = c;

        return s;
    }

    // Character wanted not found.
    return 0;

}

// ANCHOR validateWord()
// This function returns the index (if present, otherwise -1) of a searched word
// (passed by the char* word arg) in the "words_valid".
int validateWord(char* word) {

    // Dictionary not previous loaded or validated.
    if (words == NULL || words_valid == NULL) {
        // Error
        fprintf(stderr, "Error in validateWord(), call before loadDictionary() and validateDictionary().\n");
        exit(EXIT_FAILURE);
    }

    // Empty word.
    if (word == NULL) {
        // Error
        fprintf(stderr, "Error in validateWord(), NULL word received.\n");
        exit(EXIT_FAILURE);
    }

    // Word length.
    uli wl = strlen(word);
    // 'Qu' correction.
    uli cs = wl;
    for (uli i = 0LU; i < cs; i++)
        if (i + 1 != cs && word[i] == 'Q' && word[i + 1] == 'U') wl--;
    // If is set a constraint (WORD_LEN > 0) on the minimum word length, it is applied.
    if (WORD_LEN > 0LU && wl < WORD_LEN) return -1;

    // The client submitted word is totally converted
    // to the UPPERCASE version, format in which the characters in the game matrix, 
    // the words in the dictionary file and the ALPHABET used to generate random matrices use.
    // There will be no difference between a client input like "home"
    // or "HoMe" or "HOME".
    // Will all be accepted if the word is present in the dictionary file and in the
    // current game matrix.
    toLowerOrUpperString(word, 'U');

    // Searching the word in the dictionary containing only the the current game matrix words.
    for (uli i = 0; i < words_len; i++)
        if (words_valid[i][0] != '\0' && strcmp(words_valid[i], word) == 0) return i;
    
    // If we arrive here, the word is not valid.
    return -1;

}

// ANCHOR disconnectClient()
// This function disconnects a client.
// It removes the client from the clients list.
// It also frees all the heap allocated objects and destroys the relative mutex.
// It also sets to NULL the value of the pointer to the caller's client. This to avoid errors
// by reference to memory that has already been freed.
// It takes as input a struct ClientNode** clienttodestroy that rapresent the interested client.
// It also takes as input a char terminatethread. This is 1 if we want to TERMINATE the 
// CALLER thread, so it's used when the function is called itself by the client's thread that
// wants to be disconnected. This is 0 otherwise, for example if this function is called by the
// main thread in acceptClient() and we DON'T WANT TO TERMINATE the thread.
void disconnectClient(struct ClientNode** clienttodestroy, char terminatethread) {

    int retvalue;

    if (clienttodestroy == NULL) {
        // Error
        fprintf(stderr, "Error in disconnectClient(), NULL client received.\n");
        exit(EXIT_FAILURE);
    }

    struct ClientNode* client = *clienttodestroy;

    // Invalid client.
    if (client == NULL) {
        // Error
        fprintf(stderr, "Error in disconnectClient(), NULL client received.\n");
        exit(EXIT_FAILURE);
    }

    // Printing the disconnecting client's infos.
    char* strclient = serializeStrClient(client);
    fprintf(stdout, "DISCONNECTION START: %s", strclient);
    free(strclient);
    strclient = NULL;

    // Notifying the signalsThread() thread of this client's disconnection.
    client->toexit = 1;

    while (1) {
        client->waiting = 1;
        // Trylock needed.
        retvalue = pthread_mutex_trylock(&pausemutex);
        if (retvalue != 0) {
            if (retvalue == EBUSY) {
                // PAUSE IS ON!          
                sleep(1);
                continue;
            }else{
                // Error
                 fprintf(stderr, "Error in disconnectClient(), in trylock.\n");
                exit(EXIT_FAILURE);
            }
        }else break;
    }

    // pausemutex trylock succeded.

    // OWNING pause MUTEX.
    
    // Now possible danger situations cannot happen because signalsThread() and
    // eventually others clientHandler() are locked out (waiting on pausemutex)
    // and cannot acquire the listmutex.

    // Removing client from the list (the client destruction will be done LATER in this function).
    retvalue = pthread_mutex_lock(&listmutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in disconnectClient(), in locking listmutex.\n");
        exit(EXIT_FAILURE);
    }

    char found = 0;
    // Empty list (impossible if a client is requested to be disconnected).
    if (head == NULL || tail == NULL) {
        // Error
        fprintf(stderr, "Error in disconnectClient(), empty clients list.\n");
        exit(EXIT_FAILURE);
    }

    // Going trought the clients list.
    struct ClientNode* current = head;
    struct ClientNode* prev = head;
    struct ClientNode* tmp = NULL;
    while (1) {
        // Found.
        if (current == client){
            if (current == head && current == tail) found = 1;
            else if (current == head) found = 2;
            else if (current == tail) found = 3;
            else found = 4;
            break;
        }
        // End of list reached.
        if (current == NULL){
            // Error
            // The disconnected client is not in the list!?
            fprintf(stderr, "Error in disconnectClient(), the disconnected client was not found in the clients list.\n");
            exit(EXIT_FAILURE);
        }
        tmp = current;
        current = current->next;
        prev = tmp;
    }
    if (found){
        if (found == 1) {
            // The list contains only 1 element.
            head = NULL;
            tail = NULL;
        }else if (found == 2) {
            // The list contains 2 elements.
            // To remove the first element.
            head = head->next;
        }else if (found == 3) {
            // The list contains 2 elements.
            // prev->next contains the element to remove.
            // To remove the last element of the clients list.
            tail = prev;
            tail->next = NULL;
        }else{
            // At least 3 elements in the list and the victim is in the middle.
            // prev->next contains the element to remove.
            prev->next = current->next;
        }
    }else{
        // Error
        retvalue = pthread_mutex_unlock(&pausemutex);
        if (retvalue != 0) {
            // Error
            fprintf(stderr, "Error in disconnectClient(), in unlocking pausemutex.\n");
            exit(EXIT_FAILURE);
        }
        retvalue = pthread_mutex_unlock(&listmutex);
        if (retvalue != 0) {
            // Error
            fprintf(stderr, "Error in disconnectClient(), in unlocking listmutex.\n");
            exit(EXIT_FAILURE);
        }
        fprintf(stderr, "Error in disconnectClient(), the disconnected client was not found in the clients list.\n");
        exit(EXIT_FAILURE);
    }

    nclientsconnected--;

    retvalue = pthread_mutex_unlock(&pausemutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in disconnectClient(), in unlocking pausemutex.\n");
        exit(EXIT_FAILURE);
    }
    retvalue = pthread_mutex_unlock(&listmutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in disconnectClient(), in unlocking listmutex.\n");
        exit(EXIT_FAILURE);
    }

    // Cleaning the client object.
    client->client_address_len = 0;
    client->next = NULL;
    retvalue = pthread_mutex_destroy(&(client->handlerequest));
    if (retvalue != 0) {
        // Error
            fprintf(stderr, "Error in disconnectClient(), in destroying mutex.\n");
            exit(EXIT_FAILURE);
    }
    client->points = 0LU;
    free(client->words_validated);
    client->words_validated = NULL;
    free(client->name);
    client->name = NULL;
    free(client->registerafter);
    client->registerafter = NULL;
    client->actionstoexecute = 0;
    client->receivedsignal = 0;
    client->waiting = 0;
    client->toexit = 0;
    client->filledqueue = 0;

    // Closing socket.
    if (client->socket_client_fd != -1) {
        retvalue = close(client->socket_client_fd);
        if (retvalue == -1){
            // Error
            fprintf(stderr, "Error in disconnectClient(), in closing socket connection.\n");
            exit(EXIT_FAILURE);
        }else client->socket_client_fd = -1;
    }

    uli tmpt = client->threadstarted == 0 ? 0LU : (uli) client->thread;

    free(client);
    client = NULL;

    *clienttodestroy = NULL;

    if (tmpt != 0LU)
        fprintf(stdout, "DISCONNECTION COMPLETED: %lu (ID) client has disconnected succesfully.\n", tmpt);
    else
         fprintf(stdout, "DISCONNECTION COMPLETED: NO REGISTERED (NO ID) client has disconnected succesfully.\n");

    if (terminatethread) pthread_exit(NULL);
    else return;

    }

// ANCHOR endGame()
// This function is the SIGUSR1 signal handler.
// It's executed by every clientHandler() thread, when received the signal SIGUSR1 from
// the signalsThread() thread to notify the end of game.
// The signum will contain the signal's number, but it's not used.
void endGame(int signum) {

    // This is used to interrupt the read() in receiveMessage() in clientHandler() thread
    // with EINTR in errno.

    // With this we will respond to the signalsThread() thread having received the signal.
    *threadsignalreceivedglobal = 1;
    return;

}

// ANCHOR updateClients()
// Update players infos at start of a new game.
// Resetting points and setup the "words_validated", filling it with the words present in
// the current game matrix (using "words_valid").
// IT ASSUMES that needed mutexes are ALREADY LOCKED BY THE CALLER!
void updateClients(void) {

    // loadDictionary() and/or validateDictionary() not called before.
    if (words == NULL || words_valid == NULL) {
        // Error
        fprintf(stderr, "Error in updateClients(), call loadDictionary() and validateDictionary() before.\n");
        exit(EXIT_FAILURE);
    }
    
    // Going through the list.
    struct ClientNode* current = head;
    while (1) {
        if (current == NULL) break;

        // Resetting points.
        current->points = 0LU;
        // IMPORTANT: Copying the new "words_valid" (of the new matrix).
        if (current->name != NULL)
            for (uli i = 0LU; i < words_len; i++) (current->words_validated)[i] = words_valid[i];
        
        current = current->next;
    }

    fprintf(stdout, "Clients updated succesfully! Online %lu players.\nLet's play... :)\n", nclientsconnected);

}

// ANCHOR serializeStrClient()
// This function takes as input a struct ClientNode* c, rapresenting a client.
// It allocates on the heap a string, it fills it with all the client's informations, and
// finally returns the heap allocated string pointer.
char* serializeStrClient(struct ClientNode* c) {

    if (c == NULL) {
        // Error
        // printf(NULL) -> SIGSEGV so cannot trying to continue returning NULL.
        fprintf(stderr, "Error in serializeStrClient(), NULL client received.\n");
        exit(EXIT_FAILURE);
    }

    // Preparing the string format.
    char st[] = "Name: %s - Port: %lu - Points %lu - Thread ID: %lu.\n";
    char stm[] = "Name:  - Port:  - Points  - Thread ID: .\n";

    // Calculating name length.
    uli namelen;
    char* name;
    if (c->name != NULL) name = c->name;
    else name = NO_NAME;
    namelen = strlen(name);
    
    // Getting port.
    uli port = (uli) ntohs((c->client_addr).sin_port);
    // Port to string.
    char* portstr = itoa(port);
    // Calculating port length (as string).
    uli portlen = strlen(portstr);
    free(portstr);
    portstr = NULL;

    // Calculating points length (as string).
    char* pointsstr = itoa(c->points);
    uli pointslen = strlen(pointsstr);
    free(pointsstr);
    pointsstr = NULL;

    // Calculating Thread ID length (as string).
    char* threadidstr;
    if (c->threadstarted) threadidstr = itoa((uli) c->thread);
    else threadidstr = itoa(0LU);
    uli threadidstrlen = strlen(threadidstr);
    free(threadidstr);
    threadidstr = NULL;

    // Allocating the needed heap memory to store the string.
    // +1 for '\0'.
    uli totallength =  strlen(stm) + namelen + portlen + pointslen + threadidstrlen + 1;
    char* rs = (char*) malloc(totallength * sizeof(char));
    if (rs == NULL) {
        // Error
         fprintf(stderr, "Error in serializeStrClient(), malloc().\n");
        exit(EXIT_FAILURE);
    }

    // Filling the string with data.
    sprintf(rs, st, name, port, c->points, c->threadstarted ? (uli) c->thread : 0LU);

    rs[totallength - 1] = '\0';

    return rs;

}

// ANCHOR createScoreboard()
// This function should be called at the end of the scorer() thread in the end game phase.
// It creates the end game final scoreboard string.
// The format is CSV with "playername,playerpoints".
// In this format will be sent to all the clients by each clientHandler() thread.
// Each clientHandler() thread will process it to prints it in a pretty way.
// The function allocates the string on the heap and set the char* "scoreboardstr"
// global var to it.
// It takes as input an array of Queue* (Queue**) created in the scorer() thread
// and its length (as second arg).
// WARNING: Since the unregistered players cannot submits words and so scores, they are
// NOT in this scoreboard that will be sent to all the clients (them included). 
// Anyway, the unregistered players
// are printed in the server output (before of calling this function from the scorer() thread)
// for logging and debugging purpose.
void createScoreboard(struct Queue** array, uli arraylength) {

    // Args check.
    if (array == NULL || *array == NULL) {
        // Error
        fprintf(stderr, "Error in createScoreboard(), invalid args received.\n");
        exit(EXIT_FAILURE);
    }

    // Calculating the string length and allocating the corresponding heap space.
    uli totallength = 0LU;
    uli counter = 0LU;
    for (uli i = 0LU; i < arraylength; i++) 
        if (array[i]->client->name != NULL){
            totallength += strlen(array[i]->message->data);
            counter++;
        }
    // For ',' after points.
    // (next couple "playername,playerpoints|playername,playerpoints",
    // replace the '|' in the precited example).
    // This is NOT executed when the scoreboard is empty, otherwise 0 - 1 = -1. Negative length!
    if (counter != 0LU) totallength += counter - 1;
    totallength++; // For the '\0'.
    scoreboardstr = (char*) malloc(sizeof(char) * totallength);
    if (scoreboardstr == NULL) {
        // Error
        fprintf(stderr, "Error in createScoreboard(), main malloc() failed.\n");
        exit(EXIT_FAILURE);
    }

    // Copying all data from the Queue** (array) by accessing to each "message->data" field.
    counter = 0LU;
    for (uli i = 0LU; i < arraylength; i++) 
        if (array[i]->client->name != NULL) {
            char* s = array[i]->message->data;
            while (s[0] != '\0'){
                scoreboardstr[counter++] = s[0];
                s++;
            } 
            scoreboardstr[counter++] = ',';
        }
    if (counter == 0LU) counter++;
    // Inserting string terminator, replacing the last ','.
    scoreboardstr[counter - 1] = '\0';

    // Empty scoreboard check.
    if (strlen(scoreboardstr) == 0) {
        free(scoreboardstr);
        scoreboardstr = NULL;
        uli l = strlen(EMPTY_SCOREBOARD_MESSAGE_STR) + 1; // +1 for the '\0'. 
        scoreboardstr = (char*) malloc(sizeof(char) * l);
        if (scoreboardstr == NULL) {
            // Error
            fprintf(stderr, "Error in createScoreboard(), empty malloc() failed.\n");
            exit(EXIT_FAILURE);
        }
        strcpy(scoreboardstr, EMPTY_SCOREBOARD_MESSAGE_STR);
        scoreboardstr[strlen(EMPTY_SCOREBOARD_MESSAGE_STR)] = '\0';
    }

}

// ANCHOR gameEndQueue()
// This function create (and add an element to) a heap allocated queue.
// Each struct Queue element contains a struct ClientNode* object,
// a struct Message* object and a pointer to the next queue element.
// It's called by each clientHandler() thread at end of game.
// Is possible to use the queue by accessing to "headq" and "tailq" global struct Queue* that 
// rapresent respectively the head and the tail.
// It takes as input a struct ClientNode* pointer rapresenting the client to add to the queue.
// The struct Message will contain in the "data" field a CSV string message with the
// format "playername,playerpoints".
// It's required by the project's text.
void gameEndQueue(struct ClientNode* e) {

    // Invalid client.
    if (e == NULL) {
        // Error
        fprintf(stderr, "Error in gameEndQueue(), NULL client received.\n");
        exit(EXIT_FAILURE);
    }

    // Creating and filling message object.

    // Allocating space for the message on the heap.
    struct Message* m = (struct Message*) malloc(sizeof(struct Message));
    if (m == NULL) {
        // Error
        fprintf(stderr, "Error in gameEndQueue(), in malloc() of the message.\n");
        exit(EXIT_FAILURE);
    }

    m->type = MSG_PUNTI_FINALI;

    // Casting points to string and calculating points length (as string).
    char* p = itoa(e->points);
    uli plength = strlen(p);

    // For the character ','.
    plength++;
    // For the character '\0'.
    plength++;

    // Calculating name string length and total string length.
    uli namelength = e->name == NULL ? strlen(NO_NAME) : strlen(e->name);
    uli totallength = namelength + plength;
    m->length = totallength;

    // Allocating string heap space.
    m->data = (char*) malloc(sizeof(char) * m->length);
    if (m->data == NULL) {
        // Error
        fprintf(stderr, "Error in gameEndQueue(), in malloc() of the message's data.\n");
        exit(EXIT_FAILURE);
    }

    // Creating: "playername,totalpoints" string.
    // Copying name string.
    uli counter = 0LU;
    while (1) {
        m->data[counter] = e->name == NULL ? NO_NAME[counter] : e->name[counter];
        if (m->data[counter] == '\0') break;
        counter++;
    }

    // Copying points as string.
    // '\0' substitute with ','.
    m->data[counter++] = ',';

    // Copying points.
    uli counter2 = 0LU;
    while(1) {
        m->data[counter] = p[counter2];
        if (m->data[counter] == '\0') break;
        counter++;
        counter2++;
    }

    free(p);
    p = NULL;

    // Creating and filling the new queue element.
    struct Queue* new;
    new = (struct Queue*) malloc(sizeof(struct Queue));
    if (new == NULL) {
        // Error
        fprintf(stderr, "Error in gameEndQueue(), in malloc() of the queue.\n");
        exit(EXIT_FAILURE);
    }
    // Setup the new queue element.
    new->next = NULL;
    new->client = e;
    new->message = m;

    // A simply mutex used only to sync with others threads in the queue.
    int retvalue = pthread_mutex_lock(&queuemutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in gameEndQueue(), in locking queuemutex.\n");
        exit(EXIT_FAILURE);
    }
    struct Queue* tmp = NULL;
    // Example of a queue because sometimes I confuse head and tail... xD
    //           Tail -> NULL <- Head
    // Push A -> 
    //           Tail -> A <- Head
    // Push B -> 
    //           Tail -> B A <- Head
    // Pop -> 
    //           Tail -> B <- Head
    if (headq == NULL || tailq == NULL) {
        // Empty queue.
        headq = new;
        tailq = new;
    }else {
        // Not empty queue.
        tmp = tailq;
        tailq = new;
        tailq->next = tmp;
    }
    nclientsqueuedone++;
    e->filledqueue = 1;

    // Printing infos.
    char* namestr = csvNamePoints(m, 0);
    char* pointsstr = csvNamePoints(m, 1);

    fprintf(stdout, "PUSHED: New object pushed to the queue.\nPUSHED: Player's name: %s. Player's points: %s. Thread (ID): %lu.\n", namestr, pointsstr, (uli) e->thread);

    free(namestr);
    free(pointsstr);
    namestr = NULL;
    pointsstr = NULL;

    retvalue = pthread_mutex_unlock(&queuemutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in gameEndQueue(), in unlocking queuemutex.\n");
        exit(EXIT_FAILURE);
    }


}

// ANCHOR processReceivedRequest()
// This function is called by the clientHandler() function, so by a client's thread.
// It simply processes a received message, so a client's request.
// It takes as input (BY REFERENCE FROM clientHandler()) a pointer to the message to process.
// It takes as input also a pointer to the client who sent it.
// It returns a char to signal if the caller should sleep or not.
// The specialignorepause permits to process request received before the timer end game.
// IT ASSUMES that needed mutexes are ALREADY LOCKED BY THE CALLER!
char processReceivedRequest(struct Message** receivedfromclienthandler, struct ClientNode* client, char specialignorepause) {

        char sleepflag = 0;

        if (receivedfromclienthandler == NULL) {
            // Error
            fprintf(stderr, "Error in processReceivedRequest(), NULL message received.\n");
            exit(EXIT_FAILURE);
        }
        struct Message* received = *receivedfromclienthandler;

        // This is mandatory because a normal NULL could be received.
        if (received == NULL) return sleepflag;

        if (client == NULL) {
            // Error
            fprintf(stderr, "Error in processReceivedRequest(), NULL message received.\n");
            exit(EXIT_FAILURE);
        }

        // Processing the request.
        switch (received->type) {
        
            case MSG_MATRICE: {

                int r = registerUser(NULL, client, received);
                // Not yet authenticated.
                if (r == 0){
                    sendMessage(client->socket_client_fd, MSG_ERR, "You're not authenticated. Please sign up before.\n");
                    fprintf(stdout, "A user requested the game matrix before the auth.\n");            
                    break;
                }

                if (r == -1) {
                    // Error
                    fprintf(stderr, "Error in processReceivedRequest(), unexpcted error in received message.\n");
                    exit(EXIT_FAILURE);
                    break;
                }     

                // Pause game check.
                if (!pauseon || specialignorepause) {
                    // Sending current game matrix.
                    char* mat = serializeMatrixStr();
                    sendMessage(client->socket_client_fd, MSG_MATRICE, mat);
                    free(mat);
                    mat = NULL;
                    fprintf(stdout, "Matrix gets request from %s satisfied.\n", client->name);
                }else{
                    // The game is paused.
                    // The reply must be:
                    // MSG_TEMPO_ATTESA 'A' Time remaining to the start of a new game, pause left time.

                    // Seconds left to the end of the pause.
                    char timereturncode;
                    uli t = timeCalculator(pausetime, MSG_TEMPO_ATTESA, &timereturncode);
                    
                    // Sending MSG_TEMPO_ATTESA.
                    if (timereturncode == 0) {
                        sendMessage(client->socket_client_fd, MSG_TEMPO_ATTESA, "-1");
                        fprintf(stdout, "Matrix gets request from name %s converted in time request since the game is paused.\nTime to the next game: LATE...\n", client->name);
                    }else {
                        // Casting the number to string to send it in the char* data of the struct Message.
                        char* strint = itoa(t);
                        sendMessage(client->socket_client_fd, MSG_TEMPO_ATTESA, strint);
                        free(strint);
                        strint = NULL;
                        fprintf(stdout, "Matrix gets request from name %s converted in time request since the game is paused.\nTime to the next game: %lu.\n", client->name, t);
                    }
                    
                }

                break; 

            }case MSG_REGISTRA_UTENTE:{
         
                int r = registerUser(NULL, client, received);
                // Already logged in r == 1.
                if (r == 1){
                    sendMessage(client->socket_client_fd, MSG_ERR, "You're already authenticated.\n");
                    fprintf(stdout, "A user already registered, tried again to register, his current name: %s. Register requested name: %s.\n", client->name, received->data);
                    break;
                }

                if (r == -1) {
                    // Error
                    fprintf(stderr, "Error in processReceivedRequest(), unexpcted error in received message.\n");
                    exit(EXIT_FAILURE);
                    break;
                }

                if (received->length == 0 || received->data == NULL) {
                    // Error
                    fprintf(stderr, "Error in processReceivedRequest(), NULL data received.\n");
                    exit(EXIT_FAILURE);
                }

                // Trying to register the proposed name.
                r = registerUser(received->data, client, received);
                if (r == -1) {
                    // Error
                    fprintf(stderr, "Error in processReceivedRequest(), unexpcted error in received message.\n");
                    exit(EXIT_FAILURE);
                    break;
                }
                // Interrupted by end game.
                if (r == -2){
                    // IMPORTANT: Wihout this received is freed, but also copied in registerafter,
                    // then registerafter is copied in received and received freed AGAIN!
                    // Because of this bug I went crazy!
                    *receivedfromclienthandler = NULL;
                    sleepflag = 1;
                    break;
                } 

                if (r == -3){
                    // Name already present.
                    sendMessage(client->socket_client_fd, MSG_ERR, "Name already present in the game, please chose a different one.\n");
                    fprintf(stdout, "A user tried registering an already present name: %s.\n", received->data);
                    break;
                }

                if (r > 1) {
                    // The name contains at least one invalid char.
                    char fixed[] = "The proposed name contains %c, that's not in the alphabet.\nThe alphabet of admitted characters is:\n%s\n";
                    char fixedm[] = "The proposed name contains , that's not in the alphabet.\nThe alphabet of admitted characters is:\n\n";
                    uli totallength = strlen(fixedm) + strlen(ALPHABET) + 2; // +1 for the 'c' char. +1 for the '\0'.
                    char resstr[totallength];
                    sprintf(resstr, fixed, (char) r, ALPHABET);
                    resstr[totallength - 1] = '\0';
                    sendMessage(client->socket_client_fd, MSG_ERR, resstr);
                    fprintf(stdout, "A user tried to register the proposed name: %s. It contains the: %c character, that's invalid againist the alphabet %s.\n", received->data, (char) r, ALPHABET);
                    break;
                }

                // Here r must be -4, r == -4.
                // Registered succesfully.

                char fixed[] = "Registered correctly with name: %s. Your ID: %lu.\n";
                char fixedm[] = "Registered correctly with name: . Your ID: .\n";
                char* id = itoa((uli) client->thread);
                uli total = strlen(client->name) + strlen(fixedm) + strlen(id) + 1; // +1 for the '\0'.
                char resstr[total];
                sprintf(resstr, fixed, client->name, (uli) client->thread);
                resstr[total - 1] = '\0';
                sendMessage(client->socket_client_fd, MSG_OK, resstr);
                fprintf(stdout, "User registered succesfully, request from name %s satisfied with thread (ID): %lu.\n", client->name, (uli) client->thread);
                free(id);
                id = NULL;

                // pauseon == 0 means the game is ongoing, must send the current game matrix.
                if (!pauseon || specialignorepause) {
                    // Sending current game matrix.
                    char* mat = serializeMatrixStr();
                    sendMessage(client->socket_client_fd, MSG_MATRICE, mat);
                    free(mat);
                    mat = NULL;
                }

                // MSG_TEMPO_ATTESA == MSG_TEMPO_RESTANTE (error on text's project).
                // Sending MSG_TEMPO_ATTESA.
                char* strint = NULL;
                uli t;
                if (pauseon && specialignorepause == 0) {
                    // The game is paused.
                    // MSG_TEMPO_ATTESA 'A' Time remaining to the start of a new game, pause left time.
                    
                    // Seconds left to the end of the pause calculation.
                    char timereturncode;
                    t = timeCalculator(pausetime, MSG_TEMPO_ATTESA, &timereturncode);
                    if (timereturncode == 0) {
                        sendMessage(client->socket_client_fd, MSG_TEMPO_ATTESA, "-1");
                        fprintf(stdout, "Game in pause during new user signing up. Sent seconds left to the end of the pause (next game): LATE...\n");
                    }else{
                        // Casting the number to string to send it in the char* data of the struct Message.
                        strint = itoa(t);
                        sendMessage(client->socket_client_fd, MSG_TEMPO_ATTESA, strint);
                        fprintf(stdout, "Game in pause during new user signing up. Sent seconds left to the end of the pause (next game): %lu.\n", t);
                    }

                }else{
                    // The game is ongoing.
                    // MSG_TEMPO_PARTITA 'T' Time to end game.

                    // Seconds left to the end of the game calculation.
                    t = timeCalculator(matchtime, MSG_TEMPO_PARTITA, NULL);
                    fprintf(stdout, "Game ongoing during new user signing up. Sent seconds left to the end of the game: %lu.\n", t);

                    // Casting the number to string to send it in the char* data of the struct Message.
                    strint = itoa(t);
                    sendMessage(client->socket_client_fd, MSG_TEMPO_PARTITA, strint);
                    
                }

                free(strint);
                strint = NULL;

                break;

            }case MSG_PAROLA: {

                // Not logged in.
                int r = registerUser(NULL, client, received);
                if (r == 0) {
                    sendMessage(client->socket_client_fd, MSG_ERR, "You're not authenticated. Please sign up before.\n");
                    fprintf(stdout, "A user tried to submit a word before the auth.\n");            
                    break;
                }

                if (r == -1) {
                    // Error
                    fprintf(stderr, "Error in processReceivedRequest(), unexpcted error in received message.\n");
                    exit(EXIT_FAILURE);
                }

                // Game in pause.
                if (pauseon && specialignorepause == 0) {
                    sendMessage(client->socket_client_fd, MSG_ERR, "Cannot submit words during the game pause. Please, wait a new game.\n");
                    fprintf(stdout, "User with name %s, submitted word \"%s\" during the game pause. We'll be ignored.\n", client->name, received->data);  
                    break;
                }

                if (received->length == 0 || received->data == NULL) {
                    // Error
                    fprintf(stderr, "Error in processReceivedRequest(), NULL data received.\n");
                    exit(EXIT_FAILURE);
                }

                // Submitting the word.
                int p = submitWord(client, received->data);

                if (p == -1) {
                    // p == -1. Invalid word. Not present in the current game matrix or WORD_LEN violated.
                    sendMessage(client->socket_client_fd, MSG_ERR, "Invalid word. Try again... Good luck!\n");
                    fprintf(stdout, "User with name %s, submitted a IN-valid word \"%s\". Assigned 0 points, current total: %lu.\n", client->name, received->data, client->points);  
                    break;
                }
                
                // Casting int p to long unsigned is safe because the case p == -1
                // is already handled above, so here p >= 0.
                // Note however that sizeof(int) < sizeof(unsigned long int).
                char* strint = itoa((uli) p);
                if (p > 0){
                    // Valid word, never submitted before, p are the achieved points for the word length.
                    sendMessage(client->socket_client_fd, MSG_PUNTI_PAROLA, strint);
                    fprintf(stdout, "User with name %s, submitted a VALID, and never submitted before, word \"%s\". Assigned %lu points, current total: %lu.\n", client->name, received->data, (uli) p, client->points);  
                }else{
                    // Already submitted word, p == 0.
                    sendMessage(client->socket_client_fd, MSG_PUNTI_PAROLA, strint);
                    fprintf(stdout, "User with name %s, RE-submitted a valid word \"%s\". Assigned %lu points, current total: %lu.\n", client->name, received->data, (uli) p, client->points);  
                }

                free(strint);
                strint = NULL;
                break;

            }case MSG_ESCI : {
                fprintf(stdout, "Received a disconnection request from thread (ID): %lu.\n", (uli) client->thread);
                // WARNING: We will NOT RETURN to the caller!
                destroyMessage(&received);
                int retvalue = pthread_mutex_unlock(&(client->handlerequest));
                if (retvalue != 0) {
                    // Error
                    fprintf(stderr, "Error in processReceivedRequest(), in unlocking handlerequest.\n");
                    exit(EXIT_FAILURE);
                }
                disconnectClient(&client, 1);
                // THIS THREAD SHOULD NOW BE DEAD.
            }case MSG_PING_ONLINE : { 
                // Nothing to do.
                ;
                break;
            }case MSG_ERR :
            case MSG_OK:
            case MSG_TEMPO_ATTESA:
            case MSG_TEMPO_PARTITA:
            case MSG_PUNTI_PAROLA:
            case MSG_PUNTI_FINALI: {
                // Error
                // Ignoring
                break;
            }default:{
                // Error
                // Not recognized message's type. Not block all the game.
                break;
            } 

        } // Switch end.

        return sleepflag;

}

// ANCHOR gamePauseAndNewGame()
// This function is executed by a dedicated thread. This thread is started by the 
// signalsThread() thread during the pause phase. This thread sleeps for the pause
// duration and then starts a new game. These actions cannot be performed by the signalsThread()
// thread because this would block the signals management (during the thread sleeps).
void* gamePauseAndNewGame(void* args) {

    int retvalue;
    char* banner;
    struct ClientNode* current;
    fprintf(stdout, "I'm the gamePauseAndNewGame() pthread (ID): %lu.\n", (uli) gamepauseandnewgamethread);
    pthread_setname_np(gamepauseandnewgamethread, "GPANGThread");

    //////////////////  EXECUTING PAUSE  //////////////////
    retvalue = pthread_mutex_lock(&listmutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in gamePauseAndNewGame(), in locking listmutex.\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "Pause sleeping started. Online %lu players.\nSleeping zzz...\n", nclientsconnected);
    retvalue = pthread_mutex_unlock(&listmutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in gamePauseAndNewGame(), in locking listmutex.\n");
        exit(EXIT_FAILURE);
    }
    // Executing the pause.
    sleep(pauseduration);
    fprintf(stdout, "Pause sleeping finished.\n");










    //////////////////  STARTING A NEW GAME  //////////////////

    // Disabling pause and starting a new game.
    retvalue = pthread_mutex_lock(&pausemutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in gamePauseAndNewGame(), in locking pausemutex.\n");
        exit(EXIT_FAILURE);
    }
    retvalue = pthread_mutex_lock(&listmutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in gamePauseAndNewGame(), in locking listmutex.\n");
        exit(EXIT_FAILURE);
    }
    retvalue = pthread_mutex_lock(&printmutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in gamePauseAndNewGame(), in locking printmutex.\n");
        exit(EXIT_FAILURE);
    }

    // Banner of closing previous "END GAME STARTED".
    banner = bannerCreator(BANNER_LENGTH, BANNER_NSPACES, "END GAME STARTED", BANNER_SYMBOL, 1);
    fprintf(stdout, "%s\n", banner);
    free(banner);
    banner = NULL;

    // Locking threads to avoid unsafe multithreading situations.
    current = head;
    while (1) {
        if (current == NULL) break;

        retvalue = pthread_mutex_lock(&(current->handlerequest));
        if (retvalue != 0) {
            // Error
            fprintf(stderr, "Error in gamePauseAndNewGame(), in locking handlerequest.\n");
            exit(EXIT_FAILURE);
        }

        current = current->next;
    }

    // Starting a new game.
    startGame();

    // Preparing clients for a new game.
    // IMPORTANT: Call updateClients() AFTER startGame() to avoid insubstantial "words_validated".
    updateClients();

    // Disabling pause.
    pauseon = 0;

    current = head;
    while (1) {

        if (current == NULL) break;

        // Creating and sending to clients new game infos.
        banner = bannerCreator(BANNER_LENGTH, BANNER_NSPACES, "END GAME", BANNER_SYMBOL, 1);
        uli l = strlen(banner) + 1 + 1; // +1 for the '\n'. +1 for the '\0'.
        char fsm[l]; 
        sprintf(fsm, "%s%c", banner, '\n');
        fsm[l - 1] = '\0';
        free(banner);
        banner = NULL;

        banner = bannerCreator(BANNER_LENGTH, BANNER_NSPACES, "START GAME", BANNER_SYMBOL, 0);
        char pre[] = "New game matrix:\n";
        char* m = serializeMatrixStr();
        if (current->name != NULL)
            l = strlen(banner) + strlen(pre) + strlen(m) + 1 + 1; // +1 for the '\n' of end banner. +1 for the '\0'.
        else
            l = strlen(banner) + 1 + 1; // +1 for the '\n' of end banner. +1 for the '\0'.
        char msg[l];
        if (current->name != NULL)
            sprintf(msg, "%s%c%s%s", banner, '\n', pre, m);
        else
            sprintf(msg, "%s%c", banner, '\n');
        msg[l - 1] = '\0';
        free(banner);
        free(m);
        banner = NULL;
        m = NULL;

        l = strlen(msg) + strlen(fsm) + 1;
        char finalmsg[l];
        sprintf(finalmsg, "%s%s", fsm, msg);
        finalmsg[l - 1] = '\0';

        char messagetype = current->name != NULL ? MSG_MATRICE : MSG_OK;  
        sendMessage(current->socket_client_fd, messagetype, msg);

        current = current->next;

    }

    // Releasing clients locks.
    current = head;
    while (1) {
        if (current == NULL) break;
        retvalue = pthread_mutex_unlock(&(current->handlerequest));
        if (retvalue != 0) {
            // Error
            fprintf(stderr, "Error in gamePauseAndNewGame(), in unlocking handlerequest.\n");
            exit(EXIT_FAILURE);
        }
        current = current->next;
    }

    fflush(stdout);

    retvalue = pthread_mutex_unlock(&printmutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in gamePauseAndNewGame(), in unlocking printmutex.\n");
        exit(EXIT_FAILURE);
    }
    retvalue = pthread_mutex_unlock(&listmutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in gamePauseAndNewGame(), in unlocking listmutex.\n");
        exit(EXIT_FAILURE);
    }
    retvalue = pthread_mutex_unlock(&pausemutex);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error in gamePauseAndNewGame(), in unlocking pausemutex.\n");
        exit(EXIT_FAILURE);
    }

    pthread_exit(NULL);
    return NULL;

}

/*

##########################              EXAMPLE             ##########################

Let's assume that loadDictionary("/path/to/file.txt") has been called, now we will
have "char** words" global var, filled with all the words present in the dictionary
file "/path/to/file.txt" (assuming a word for line terminated by \n or by \r\n).

Being the content of /path/to/file.txt:
hello\n
dog\n
mum

Let's see the content of "char** words" after loadDictionary(...).
Allocated on heap.
char** words -> words[0] (char*) -> "hello\0"
             -> words[1] (char*) -> "dog\0"
             -> words[2] (char*) -> "mum\0"

Being the current game matrix the following:
h e D F
O l l o
N M Z m
E T m u

The characters in the matrix are all uppercase.
In the notation of this example, I wrote some lowercase characters
to denote those that form the words of interest (present in the file).
Also, let's assume there is not the constraint on the word length.

Now let's call validateDictionary(); At the end we will have "char** words_valid" global var
filled with:
char** words_valid -> words_valid[0] (char*) -> "hello\0"
                   -> words_valid[1] (char*) -> "\0"
                   -> words_valid[2] (char*) -> "mum\0"

The word "dog" is not present because it is in the dictionary file, and so in the "char** words",
but NOT in the current game matrix. Instead "hello\0" and "mum\0" are present because were
founded BOTH in the dictionary file (char** words) and the current game matrix (look at
the lowercase letters above).

The "dog\0" word was not deleted from memory (freed), we simply updated the copied pointer,
incrementing it.

Note that we are operating by exploiting the power of pointers, with their arithmetic,
without having two copies of the strings in memory, but only two arrays with their pointers (char*).

*/

/*

 ########   SOME INFORMATIONS ON SYNCHRONIZATION BETWEEN THREADS AND SIGNALS MANAGEMENT  ########
    
sigwait() handles all SIGINT, SIGALRM and SIGPIPE signals in the project running in a dedicated 
thread called signalsThread(), started in the main after blocking them for others threads
as sigwait() requires.
If a signal arrive, the sigwait() reads that signal from the signals
mask and sets to 0 (clearing) the corresponding bit and the code continues.
If during this another signal arrive, it is put in pending using pending signals mask.
This means the corresponding bit of the signals mask is set to one (previously 0).
If another signal of the same type arrive during the handling of a signal and for wich the 
corresponding bit of the signals mask has been already setted to 1, this new signal is lost,
because the bit of signals mask is already 1 and it's simply overwritten.
So the signals aren't enqueued.
Therefore, I cannot be interrupted by a new signal while handling the previous one with sigwait().
When a game is over, the alarm() trigger a SIGALRM signal caugth by signalsThread() thread.
So, the signalsThread() thread waits all clientHandler() threads to complete the current
request in processing, then it acquires their mutexes (each struct ClientNode in clients list
has its own mutex (handlerequest) that release after completing the client's request).
Then, we use a pthread_kill() call to inform each clientHandler() thread that the game is over
with a SIGUSR1 signal (not handled by signalsThread() thread, but handled by each clientHandler() thread).
In this way, all threads  blocked on the read() in receiveMessage() are "suspended" on
their own mutexes (handlerequest).
Specifically, threads locked on read() are interrupted and the read() will fail returning EINTR
in errno, and the receiveMessage() (read() caller) function will return a corresponding code
to the clientHandler() thread caller.
Now we now that all the read() are stopped and the execution of each
clientHandler() thread is waiting for the signalsThread() thread to release each handlerequest
mutex.
Now we can enable safety the pause ("pauseon" global var) trought the signalsHandler() thread,
because we will be sure that no others threads can perform unsafe multithreaded actions.
Then we release all clientHandler() mutexes (for each client, its own handlerequest).
Now each client will acquire its own lock and will detect the end of game, so it
will fill the end-game queue, which the thread scorer() will subsequently take care of as
required by the project text (started by the signalsThread() thread), by creating the CSV
final scoreboard. Afterwards each clientHandler() thread will send this scoreboard to its
handled client.
Finally all the clientHandler() threads can come back to response to all
the clients' requests considering the global var “pauseon”, that now is enabled (true),
the fact that the game is paused.

*/


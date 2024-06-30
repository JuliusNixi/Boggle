// ANCHOR File begin.
// Shared server cross files vars and libs.
#include "server.h"

// TODO Check if all this is needed.

// Current file vars and libs.
#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>

struct ClientNode* head = NULL; // Pointer to the clients list head.
struct ClientNode* tail = NULL; // Pointer to the clients list tail.
pthread_mutex_t listmutex = PTHREAD_MUTEX_INITIALIZER; // Mutex that will used from threads to synchronize interactions with the list of clients.

char** words = NULL;  // Pointer to a char[][] array that will be allocated on the heap. Each string rapresent a word/line on the dictionary file, NULL terminated.
char** words_valid = NULL;  // Copy of "words" above (char[][] array heap allocated) to be used when the dictionary is validated, will contains only the words present BOTH in the dictionary and the current game matrix. It will be updated every times that the current game matrix changes.
uli words_len = 0U;   // Length of BOTH char[][] above.

uli matchtime = 0LU;  // Time of the last game startup.
uli pausetime = 0LU;  // Time of the last pause startup.

// TODO Reset times.
#define tempteststimeseconds 8
#define PAUSE_DURATION 8 // Duration of the pause in minutes. Default 1 minute.
// For the game duration time, see gameduration var in server.h
//#define PAUSE_DURATION 7 // Duration of the pause in minutes.

// TODO Restore word_len after tests.
//#define WORD_LEN 4LU  // If set to an integer greater than 0, the server will refuse all the words that not match this length, even if present in the dictionary and in the current game matrix.
#define WORD_LEN 0LU

char pauseon = 0; // This var will be 1 if the game is paused, and 0 otherwise (game in play). It is used by the clientHandler() threads to understand how to response to the clients requests.
pthread_mutex_t pausemutex = PTHREAD_MUTEX_INITIALIZER; // This mutex will be used to synchronize threads during switching pause/game-on phases.
pthread_t pausethread; // This will be a thread dedicated to execute the game pause. It will sleep the pause duration.

#define MAX_NUM_CLIENTS 0 // This is the maximum number of connected clients to the server. It's use is optional since the clients data are stored in a unlimited (computational resources permitting) linked list.
uli nclientsconnected = 0LU; // This rapresent the number of connected clients to the server. It's include BOTH registered and unregistered users.

#define NO_NAME "unregistered" // This will be the default name assigned to unregistered players.

uli clientid = 0LU;  // A temporary client's ID used to identify a client before its thread starting.

#define VALID_WORDS_TESTS_FILE_PATH "../Tests/fileCurrentValidsWords.txt"

pthread_mutex_t queuemutex = PTHREAD_MUTEX_INITIALIZER; // This mutex will be used to synchronize threads ad the end of game to fill the queue.
struct Queue* headq = NULL; // Pointer to the queue head.
struct Queue* tailq = NULL; // Pointer to the queue tail.
uli nclientsqueuedone = 0LU; // This will store the number of clients that have already filled the queue.
pthread_t scorert; // This thread will execute the scorer() function as described in the project statement.
char* scoreboardstr = NULL; // This will containt the final CSV sorted (by player's points) scoreboard heap allocated string.
// This thread is required by the assignment details project document. This function will be executed in a dedicated thread. At end of game will pick up the content of the queue to produce the final game scoreboard. This scoreboard will be a heap allocated string pointed by scoreboardstr (above var). The scoreboard's format will be CSV (comma separeted values), sorted by player's points (descending). All clientHandler() threads will send this scoreboard to each player (BOTH registered or not). This thread will also determine the winner and will print it.
// ANCHOR scorer()
void* scorer(void* args) {


    ////////////////////////////////////////////////////////////////////////////////////////
    // CAUTION: NOW (at execution of this thread scorer()) clientHandler() THREADS        //
    // ARE RUNNING AGAIN AND CAN ACCESS TO THEIR respective                               //
    // struct ClientNode* OBJECT (but not the entire clients list)!                       //
    // However the pausemutex and listmutex MUST BE LOCKED BEFORE STARTING THIS THREAD!   //
    ////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////// FOR TESTING ONLY //////////////////////////////

    // CREATING A RANDOM QUEUE FOR TESTING

    /*
    int clients = 30;
    nclientsconnected = clients;

    time_t t = time(NULL);
    printf("TIME: %llu.\n", (unsigned long long) t);
    srand(t);
    //srand(1717086224);
    
    for (int i = 0; i < clients; i++) {

        struct ClientNode* new = NULL;
        new = (struct ClientNode*) malloc(sizeof(struct ClientNode));
        new->client_address_len = (socklen_t) sizeof(new->client_addr);
        new->socket_client_fd = -1;
        new->next = NULL;
        pthread_mutexattr_init(&(new->handlerequestattr));
        pthread_mutexattr_settype(&(new->handlerequestattr), PTHREAD_MUTEX_ERRORCHECK);
        pthread_mutex_init(&(new->handlerequest), &(new->handlerequestattr));

        // Rand points.
        int randint = rand() % 100;
        new->points = (unsigned int) randint;

        new->words_validated = NULL;
        new->name = NULL;
        new->registerafter = NULL;
        new->actionstoexecute = 0U;

        char sip[] = "127.0.0.1";
        parseIP(sip, &(new->client_addr));
        char buf[INET_ADDRSTRLEN] = {0}; 
        inet_ntop(AF_INET, &(new->client_addr.sin_addr), buf, new->client_address_len);
        unsigned int port = atoi("80");
        (new->client_addr).sin_port = htons(port);
        
        new->thread = (pthread_t) 0LU;

        // Rand name.
        int r = rand() % 10;
        if (r >= 5) {
            char n[6];
            for (int j = 0; j < 5; j++) n[j] = ALPHABET[rand() % strlen(ALPHABET)];
            n[6-1] = '\0';
            new->name = (char*) malloc(sizeof(char) * strlen(n));
            strcpy(new->name, n);
        }

        gameEndQueue(new);

    }
    //////////////////////////////////////////////////////////////////////////////
    */

   printff(NULL, 0, "I'm the scorer pthread (ID): %lu.\n", (uli) pthread_self());
   // Setup thread destructor.
   threadSetup();

    // No players, no one to send scoreboard... :(
    if (nclientsqueuedone == 0LU){
        printff(NULL, 0, "No players... :(\n");
        pthread_exit(NULL);
    } 


    // Copying clients list in an array to use qsort.
    struct Queue* array[nclientsqueuedone];
    struct Queue* currentl = tailq;
    uli counter = 0LU;
    while (1) {
        if (currentl == NULL) break;
        array[counter++] = currentl;
        currentl = currentl->next;
    }

    // Sorting players by points using message data.
    // To use qsort i copied the list in the temporary array.
     
    qsort(array, nclientsqueuedone, sizeof(struct Queue*), sortPlayersByPointsMessage);
  /*/  mLock(&mutexprint);
    printff(NULL, 1, "\n\t\tFINAL SCOREBOARD\n");
    for (unsigned int i = 0U; i < nclientsqueuedone; i++) {
        char* s = serializeStrClient(array[i]->client);
        printff(NULL, 1, "%s", s);
        free(s);
    }
    printff(NULL, 1, "\n");
    mULock(&mutexprint);*/


    // Taking from the previously sorted array the maximum number of points.
    char* pstr = csvNamePoints(array[0]->message, 1);
    uli p = (uli) strtoul(pstr, NULL, 10);;
    free(pstr);
    // All players have 0 points.
    if (p == 0LU) {
        printff(NULL, 0, "No winners, all players have 0 points.\n");
    }

    // Counting the number of players that scored the same maximum points.
    counter = 0LU;
    // Current points temp var.
    uli cp;
    for (unsigned int i = 0; i < nclientsqueuedone; i++) {
        // Skipping this code if all players have 0 points.
        // Remember p is the maximum scored points by players.
        if (p == 0LU) break;
        // Getting points.
        pstr = csvNamePoints(array[i]->message, 1);
        cp = (uli) strtoul(pstr, NULL, 10);;
        free(pstr);
        // Counting the number of players that scored the same maximum points.
        if (p == cp) counter++;
        else break;
    }
    // Only one winner with maximum p points.
    mLock(&mutexprint);
    if (counter == 1LU) {
        char* n = csvNamePoints(array[0]->message, 0);
        printff(NULL, 1, "The winner is: %s with %lu points.\n", n, p);
        free(n);
    }

    if (p != 0LU && counter != 1LU) {
        printff(NULL, 1, "The winners with %lu points are:\n", p);
        for (unsigned int i = 0; i < nclientsqueuedone; i++) {
            // Getting maximum "p" points.
            pstr = csvNamePoints(array[i]->message, 1);
            cp = (uli) strtoul(pstr, NULL, 10);
            free(pstr);
            // Printing multiple winners with same maximum "p" points scored.
            if (p == cp){
                char* n = csvNamePoints(array[i]->message, 0);
                printff(NULL, 1, "%s\n", n);
                free(n);
            }
            else break;
        }
    }
    mULock(&mutexprint);

    // Creating and printing the CSV final game scoreboard.
    createScoreboard(array, nclientsqueuedone);

    printff(NULL, 0, "\nHere the CSV scoreboard that will be sent to all clients:\n%s\n\n", scoreboardstr);
        
    pthread_exit(NULL);

}

// ANCHOR csvNamePoints()
// This function take as input a message.
// The message MUST contains in the "data" field a string in the format "playername,playerpoints"
// that will be tokenized.
// This function will return the player's name or the player's points.
// The second function arg is a char. It can value 1 if we want the "playername", 0 if we want
// the "playerpoints". In both cases the function will return a char* heap allocated string.
// This function is used in the scorer() thread.
char* csvNamePoints(struct Message* m, char nameorpoints) {

    if (m == NULL) {
        // Error
        handleError(0, 0, 0, 1, "Error in csvNamePoints(), NULL message received.\n");
    }

    if (nameorpoints != 0 && nameorpoints != 1) {
        // Error
        handleError(0, 0, 0, 1, "Error in csvNamePoints(), \"nameorpoints\" could be 0 or 1.\n");
    }

    // Copying message "data" field in a temporary string to use strtok().
    char* s = m->data;
    char* backup = (char*) malloc(sizeof(char) * (strlen(s) + 1));
    if (backup == NULL) {
        // Error
        handleError(0, 0, 0, 1, "Error in csvNamePoints(), in malloc() backup.\n");
    }
    strcpy(backup, s);
    // Terminating string.
    backup[strlen(s)] = '\0';

    // Getting name.
    char* tmp = strtok(backup, ",");
    uli namelen = strlen(tmp) + 1;
    char* name = (char*) malloc(sizeof(char) * namelen);
    if (name == NULL) {
        // Error
        handleError(0, 0, 0, 1, "Error in csvNamePoints(), in malloc() name.\n");
    }
    strcpy(name, tmp);
    // Terminating string.
    name[strlen(tmp)] = '\0';

    // Getting points.
    tmp = strtok(NULL, ",");
    uli pointslen = strlen(tmp) + 1;
    char* points = (char*) malloc(sizeof(char) * pointslen);
    if (points == NULL) {
        // Error
        handleError(0, 0, 0, 1, "Error in csvNamePoints(), in malloc() points.\n");
    }
    strcpy(points, tmp);        
    // Terminating string.
    points[strlen(tmp)] = '\0';

    char* ret;
    if (nameorpoints == 0) {
        // Wanted name.
        free(points);
        ret = name;
    }else{
        // Wanted points.
        free(name);
        ret = points;
    }

    free(backup);

    return ret;

}

// ANCHOR sortPlayersByPointsMessage()
// This function is used in qsort (in the scorer() thread) to sort an array of struct Queue*
// containing pointers to elements struct Queue.
// Each element contains a message with the "data" field in the format "playername,points".
// This function is used from qsort to create a sorted final game points scoreboard.
int sortPlayersByPointsMessage(const void* a, const void* b) {

    struct Queue** x = (struct Queue**) a;
    struct Queue** y = (struct Queue**) b;

    struct Queue* xx = *x;
    struct Queue* yy = *y;

    struct Message* mx = xx->message;
    struct Message* my = yy->message;

    char* s = csvNamePoints(mx, 1);
    uli px = (uli) strtoul(s, NULL, 10);
    free(s);
    s = csvNamePoints(my, 1);
    uli py = (uli) strtoul(s, NULL, 10);
    free(s);

    return py - px;

}

// ANCHOR generateRandomMatrix()
// This function generate a random letters matrix of size as written in NCOL and NROWS.
// The letters that will be used are only those present in the ALPHABET.
// matrix, ALPHABET, NROWS and NCOLS are defined in the server.h.
// The matrix (global char[NROWS][NCOL]) will be filled with these letters.
void generateRandomMatrix(void) {

    int randint;
    // Iterating on game matrix.
    for (uli i = 0; i < NROWS; i++)
        for (uli j = 0; j < NCOL; j++) {
            // Choosing a random letter from ALPHABET (global #define) and filling the matrix.
            randint = rand() % strlen(ALPHABET);
            // The characters written in the alphabet are used all in UPPERCASE version,
            // regardless how hey are written in the #define global alphabet.
            matrix[i][j] = toupper(ALPHABET[randint]);
        }

    // Validating the new game matrix.
    validateMatrix();

    fprintf(stdout, "New random matrix created and validated succesfully.\n");

}

// ANCHOR loadMatrixFromFile()
// This function fills the game matrix (matrix is a global char[NROWS][NCOL])
// of size as written in NCOL and NROWS by reading a file (with file path) received as arg from CLI.
// matrix, NROWS and NCOLS are defined in the server.h.
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
    // for that the pointer and the stat need to be static.
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
        if (MAT_PATH != NULL) free(MAT_PATH);
        // Allocating new memory for the new path.
        MAT_PATH = (char*) malloc((strlen(path) + 1) * sizeof(char));
        if (MAT_PATH == NULL) {
            // Error
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
        }
        // Check if the file is regular.
        if(!S_ISREG(s.st_mode)){
            // Error
        }

        // Releasing previous file content if present.
        if (file != NULL) {
            free(file);
            retvalue = close(fd);
            if (retvalue == -1) {
                // Error
            }
            fd = -1;
        }
            
        // Allocating heap memory for the new file content.
        file = (char*) malloc(sizeof(char) * (s.st_size + 1));
        if (file == NULL) {
            // Error
        }

        // Opening the file in readonly mode.
        fd = open(MAT_PATH, O_RDONLY, NULL);
        if (fd == -1) {
            // Error
        }

        // Reading the file content using a buffer of BUFFER_SIZE length.
        char buffer[BUFFER_SIZE];
        while (1) {
            retvalue = read(fd, buffer, BUFFER_SIZE);
            if (retvalue == -1) {
                // Error
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
    if (i == s.st_size) i = 0LU;
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
            }
            i++;
            break;
        }

        // Getting next matrix indexes to write.
        getMatrixNextIndexes(matrixnextindexes);
        if (matrixnextindexes[0] == -1){
            // Error 
            // Invalid format file.
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
// The SIGALRM will be used to trigger the game pause.
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
    }
    setupfinished++;
    retvalue = pthread_mutex_unlock(&setupmutex);
    if (retvalue != 0) {
        // Error
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
                // TODO SIGINT
                break;
            }case SIGALRM:{
                // This will manage the SIGALRM signal triggered by the timer when the game is over.

                // This is the CORE of the server.
                // ANCHOR Game Core.

                /////////////////////////   GAME OVER   /////////////////////////

                // TODO Prints all in a block, interleaved prints problem.
                char* banner = bannerCreator(BANNER_LENGTH, BANNER_NSPACES, NULL, BANNER_SYMBOL, 1);
                fprintf(stdout, "%s\n", banner);
                free(banner);

                banner = bannerCreator(BANNER_LENGTH, BANNER_NSPACES, "END GAME STARTED", BANNER_SYMBOL, 0);
                fprintf(stdout, "\n%s\n", banner);
                free(banner);
                fprintf(stdout, "The game is just ended. The requests from clients received until now will anyways be completed.\n");

                struct ClientNode* current;










                //////////////////  QUEUE FILLING SETUP AND ENABLING PAUSE  //////////////////
                
                // Enabling pause and notifying the threads handlers of the end of game, to stop read()
                // and start working on queue.
                retvalue = pthread_mutex_lock(&pausemutex);
                if (retvalue != 0) {
                    // Error
                }
                retvalue = pthread_mutex_lock(&listmutex);
                if (retvalue != 0) {
                    // Error
                }
                current = head;
                while (1) {
                    if (current == NULL) break;

                    // Locking threads handlers, when they complete their current request.
                    retvalue = pthread_mutex_lock(&(current->handlerequest));
                    if (retvalue != 0) {
                        // Error
                    }

                    // Creating end game message to send it to the clients.
                    banner = bannerCreator(BANNER_LENGTH, BANNER_NSPACES, "END GAME", BANNER_SYMBOL, 0);
                    uli l = strlen(banner) + 2; // +1 for '\n' and +1 for '\0'.
                    char msgendgame[l];
                    strcpy(msgendgame, banner);
                    msgendgame[l] = '\0';
                    msgendgame[l - 1] = '\n';
                    free(banner);
                    banner = NULL;
                    
                    banner = bannerCreator(BANNER_LENGTH, BANNER_NSPACES, NULL, BANNER_SYMBOL, 1);
                    l = strlen(banner) + 2; // +1 for '\n' and +1 for '\0'.
                    char msgendgameend[l];
                    strcpy(msgendgameend, banner);
                    msgendgameend[l] = '\0';
                    msgendgameend[l - 1] = '\n';
                    free(banner);
                    banner = NULL;

                    l = strlen(msgendgame) + strlen(msgendgameend) + 1; // +1 for '\0'.
                    char finalmsg[l];
                    uli counter = 0LU;
                    while(1) {
                        if (msgendgame[counter] == '\0') {
                            break;
                        }
                        finalmsg[counter] =  msgendgame[counter];
                        counter++;
                    }
                    uli counter2 = 0LU;
                    while(1) {
                        if (msgendgameend[counter2] == '\0') {
                            finalmsg[counter] = '\0';
                            break;
                        }
                        finalmsg[counter] = msgendgameend[counter2];
                        counter++;
                        counter2++;
                    }
                    
                    sendMessage(current->socket_client_fd, MSG_OK, finalmsg);

                    current = current->next;

                }

                // Enabling pause, it's safe because all the clients threads are SUSPENDED
                // on their mutexes or before on read() and cannot read pauseon.
                pauseon = 1;

                current = head;
                // IMPORTANT: Reset actionstoexecute, receivedsignal and filledqueue.
                while (1) {
                    if (current == NULL) break;
                    current->actionstoexecute = 0;
                    current->receivedsignal = 0;
                    current->filledqueue = 0;
                    current = current->next;
                }

                current = head;
                // Notifying the threads handlers of end of game to stop read().
                while (1) {
                    if (current == NULL) break;
                    threadsignalreceivedglobal = &(current->receivedsignal);
                    retvalue = pthread_kill(current->thread, SIGUSR1); 
                    if (retvalue != 0) {
                        // Error
                        usleep(100);
                    } 
                    // Continuing only when the signal has been received by the client.
                    if (current->receivedsignal && current->waiting) current = current->next;
                    else usleep(100);
                }
                // Now all clients threads (in clientHandler()) are suspended
                // on their mutexes (eventually read() are stopped),
                // except for those in disconnectClient(), but these last are not relevant.
                // Releasing all threads locks, now they can read the modified pauseon and will
                // work on the shared queue.
                current = head;
                while (1) {
                    if (current == NULL) break;
                    retvalue = pthread_mutex_unlock(&(current->handlerequest));
                    if (retvalue != 0) {
                        // Error
                    }
                    current = current->next;
                }
                // Important to realease to not suspend during the pause the clients accepting.
                retvalue = pthread_mutex_unlock(&listmutex);
                if (retvalue != 0) {
                    // Error
                }










                //////////////////  ENABLED PAUSE THREADS WORKING ON QUEUE  //////////////////

                fprintf(stdout, "Pause enabled. From now all the clients requests will be threated as in end game phase.\n");

                // STILL OWNING pausemutex -> registerUser() and disconnectClient() suspended.

                //////////////////////////////////////////////////////////////////////////////
                // WARNING: NOW clientHandler() THREADS ARE RUNNING AGAIN AND CAN ACCESS TO //
                // struct ClientNode* OBJECT (their own mutex not entirely the list)!       //
                //////////////////////////////////////////////////////////////////////////////

                // REMEMBER: pauseon is still active and conditions the responses.

                // Threads working on queue...

                // Those blocked on registerUser() return to the beginning of clientHandler()
                // and can continue in this task, so there is no danger of deadlock.
                // Also for the disconnectClient() threads there is not the risk of deadlock.
                // After threads can once again return to respond to clients requests.
                fprintf(stdout, "Clients released and notified, now they should working on the queue.\n");










                //////////////////  WAITING QUEUE  //////////////////

                // Checking if threads completed their jobs on queue.
                while (1) {
                    char toexit = 0;
                    retvalue = pthread_mutex_lock(&queuemutex);
                    if (retvalue != 0) {
                        // Error
                    }
                    // nclientsconnected remember to use it only after acquiring the listmutex!
                    retvalue = pthread_mutex_lock(&listmutex);
                    if (retvalue != 0) {
                        // Error
                    }
                    uli filledqueueclients = 0LU;
                    current = head;
                    while(1) {
                        if (current == NULL) break;
                        if (current->filledqueue || current->toexit) filledqueueclients++;
                        current = current->next;
                    }
                    // All threads have succesfully filled the queue.
                    if (filledqueueclients == nclientsconnected) toexit = 1;
                    retvalue = pthread_mutex_unlock(&listmutex);
                    if (retvalue != 0) {
                        // Error
                    }
                    retvalue = pthread_mutex_unlock(&queuemutex);
                    if (retvalue != 0) {
                        // Error
                    }
                    if (toexit) break;
                    // To avoid instant re-acquiring.
                    usleep(100);
                }
                fprintf(stdout, "Queue has been succesfully filled by all the clients threads.\n");










                //////////////////  EXECUTING SCORER  //////////////////

                // Creating and executing the scorer thread.
                // Important to lock listmutex since nclientsconnected is used inside the scorer thread.
                
                // STILL OWNING pausemutex -> registerUser() and disconnectClient() suspended.

                retvalue = pthread_mutex_lock(&queuemutex);
                if (retvalue != 0) {
                    // Error
                }
                retvalue = pthread_mutex_lock(&listmutex);
                if (retvalue != 0) {
                    // Error
                }
                
                current = head;
                while (1) {
                    if (current == NULL) break;
                    retvalue = pthread_mutex_lock(&(current->handlerequest));
                    if (retvalue != 0) {
                        // Error
                    }
                    current = current->next;
                }

                current = head;
                // IMPORTANT: Reset receivedsignals and waiting.
                while (1) {
                    if (current == NULL) break;
                    current->receivedsignal = 0;
                    current->waiting = 0;
                    current = current->next;
                }

                // Creating the scorer thread.
                retvalue = pthread_create(&scorert, NULL, scorer, NULL);
                if (retvalue != 0) {
                    // Error
                }
                // Waiting the score thread to finish.
                retvalue = pthread_join(scorert, NULL);
                if (retvalue != 0){
                    // Error
                }

                current = head;
                // Notifying the threads handlers of the completed thread scorer(), to send
                // the scoreboard to their clients.
                while (1) {
                    if (current == NULL) break;
                    threadsignalreceivedglobal = &(current->receivedsignal);
                    retvalue = pthread_kill(current->thread, SIGUSR1); 
                    if (retvalue != 0) {
                        // Error
                        usleep(100);
                    }
                    if (current->receivedsignal && current->waiting == 1) current = current->next;
                    else usleep(100);
                }

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
                    retvalue = pthread_mutex_unlock(&(current->handlerequest));
                    if (retvalue != 0) {
                        // Error
                    }
                    current = current->next;
                }
                retvalue = pthread_mutex_unlock(&listmutex);
                if (retvalue != 0) {
                    // Error
                }
                retvalue = pthread_mutex_unlock(&queuemutex);
                if (retvalue != 0) {
                    // Error
                }  
                if(scoreboardstr != NULL) fprintf(stdout, "Scorer thread is terminated, the final CSV scoreboard should be filled.\n");
                else fprintf(stdout, "Scorer thread is terminated, since there are no players, there is no scoreboard... :(\n");










                //////////////////  CLIENTS END GAME WAITING COMUNICATION  //////////////////

                // Threads working on end game send message.
                // After, threads can once again return to respond to clients' requests.
                fprintf(stdout, "Clients released and notified, now they should communicate to their clients the CSV scoreboard.\n");
                while(1) {
                    char toexit = 0;
                    retvalue = pthread_mutex_lock(&listmutex);
                    if (retvalue != 0) {
                        // Error
                    }
                    current = head;
                    while (1) {
                        if (current == NULL) break;
                        retvalue = pthread_mutex_lock(&(current->handlerequest));
                        if (retvalue != 0) {
                            // Error
                        }
                        current = current->next;
                    }
                    current = head;
                    uli nclientsmessagesent = 0LU;
                    while(1) {
                        if (current == NULL) break;
                        if (current->actionstoexecute == 4 || current->toexit) {
                            // Client has sent the end game message.
                            nclientsmessagesent++;
                        }
                        current = current->next;
                    }
                    if (nclientsconnected == nclientsmessagesent) toexit = 1;
                    current = head;
                    while (1) {
                        if (current == NULL) break;
                        retvalue = pthread_mutex_unlock(&(current->handlerequest));
                        if (retvalue != 0) {
                            // Error
                        }
                        current = current->next;
                    }
                    retvalue = pthread_mutex_unlock(&listmutex);
                    if (retvalue != 0) {
                        // Error
                    }
                    if (toexit) break;
                    // To avoid instant re-acquiring.
                    usleep(100);
                }
                // Releasing scoreboardstr.
                if (scoreboardstr != NULL) free(scoreboardstr);
                scoreboardstr = NULL;
                // Releasing pausemutex, so registerUser() and disconnectClient() if
                // necessary could continue.
                retvalue = pthread_mutex_unlock(&pausemutex);
                if (retvalue != 0) {
                        // Error
                }
                // Releasing also the listmutex is important to avoid to blocks the
                // new user's socket connection acceptance during the game pause.










                //////////////////  CLEARING QUEUE  //////////////////

                // Clearing the queue, remember here the scorer thread is finished, due
                // to our pthread_join.
                // So locking this mutex now doesn't make sense, because no other thread
                // right now should be working on the shared queue, however it was done
                // for clarity.
                retvalue = pthread_mutex_lock(&queuemutex);
                if (retvalue != 0) {
                        // Error
                }
                if (headq == NULL || tailq == NULL) {
                    // No players connected...
                    // Nothing to do.
                    return;
                }
                // Freeing heap allocated memory.
                struct Queue* begin = tailq;
                struct Queue* tmp = NULL;
                while (1) {
                    if (begin == NULL) break;
                    tmp = begin->next;
                    struct Queue* t = begin;
                    if (begin->message) destroyMessage(&(begin->message));
                    if (t) free(t);
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
                }










                //////////////////  EXECUTING PAUSE  //////////////////

                fprintf(NULL, 0, "Pause sleeping started.\n");
                // Executing pause.
                fprintf(stdout, "Sleeping zzz...\n");

                // Getting new starting pause timestamp in POSIX time.
                pausetime = (uli) time(NULL);

                // Executing the pause.
                // PAUSE_DURATION in minutes, but sleep takes seconds.
                //sleep(PAUSE_DURATION * 60);
                sleep(tempteststimeseconds);
                fprintf(stdout, "Pause sleeping finished.\n");










                //////////////////  STARTING A NEW GAME  //////////////////

                // Disabling pause and starting a new game.
                retvalue = pthread_mutex_lock(&pausemutex);
                if (retvalue != 0) {
                        // Error
                }
                retvalue = pthread_mutex_lock(&listmutex);
                if (retvalue != 0) {
                        // Error
                }

                banner = bannerCreator(BANNER_LENGTH, BANNER_NSPACES, NULL, BANNER_SYMBOL, 1);
                fprintf(stdout, "%s\n", banner);
                free(banner);

                // Locking threads to avoid unsafe multithreading situations.
                current = head;
                while (1) {
                    if (current == NULL) break;
                    retvalue = pthread_mutex_lock(&(current->handlerequest));
                    if (retvalue != 0) {
                        // Error
                    }
                    current = current->next;
                }
                // Disabling pause.
                pauseon = 0;
                // Starting a new game.
                startGame();
                // Preparing clients for a new game.
                // IMPORTANT: Call updateClients() AFTER startGame() to avoid insubstantial "words_validated".
                updateClients();
                current = head;
                while (1) {
                    if (current == NULL) break;

                    // Inform clients of the start of a new game and
                    // sending the new game matrix.
                    banner = bannerCreator(BANNER_LENGTH, BANNER_NSPACES, "NEW GAME STARTED", BANNER_SYMBOL, 0);
                    uli l = strlen(banner) + 2; // +1 for '\n' and +1 for '\0'.
                    char msgstartgame[l];
                    strcpy(msgstartgame, banner);
                    msgstartgame[l] = '\0';
                    msgstartgame[l - 1] = '\n';
                    free(banner);
                    banner = NULL;

                    // Sending matrix only to registered players.
                    char* str = NULL;
                    char* matstr  = serializeMatrixStr();
                    char premessage[] = "New matrix:\n";
                    char matstrandpre[strlen(matstr) + strlen(premessage) + 1];
                    uli counter = 0LU;
                    while(1) {
                        if (premessage[counter] == '\0') break;
                        matstrandpre[counter] = premessage[counter];
                        counter++;
                    }
                    uli counter2 = 0LU;
                    while(1) {
                        if (matstr[counter2] == '\0'){
                            matstrandpre[counter] = matstr[counter2];
                            break;
                        }
                        matstrandpre[counter] = matstr[counter2];
                        counter++;
                        counter2++;
                    }
                    if (current->name != NULL)
                        str = matstrandpre;
                    else
                        str = NULL;
                    
                    banner = bannerCreator(BANNER_LENGTH, BANNER_NSPACES, NULL, BANNER_SYMBOL, 1);
                    l = strlen(banner) + 2; // +1 for '\n' and +1 for '\0'.
                    char msgstartgameend[l];
                    strcpy(msgstartgameend, banner);
                    msgstartgameend[l] = '\0';
                    msgstartgameend[l - 1] = '\n';
                    free(banner);
                    banner = NULL;

                    l = strlen(msgstartgame) + strlen(msgstartgameend) + 1; // +1 for '\0'.
                    if (str != NULL) l += strlen(str);
                    char finalmsg[l];
                    counter = 0LU;
                    while(1) {
                        if (msgstartgame[counter] == '\0') {
                            break;
                        }
                        finalmsg[counter] = msgstartgame[counter];
                        counter++;
                    }
                    counter2 = 0LU;
                    while(1) {
                        if (str == NULL) break;
                        if (str[counter2] == '\0') {
                            break;
                        }
                        finalmsg[counter] = str[counter2];
                        counter++;
                        counter2++;
                    }
                    counter2 = 0LU;
                    while(1) {
                        if (msgstartgameend[counter2] == '\0') {
                            finalmsg[counter] = '\0';
                            break;
                        }
                        finalmsg[counter] = msgstartgameend[counter2];
                        counter++;
                        counter2++;
                    }

                    sendMessage(current->socket_client_fd, MSG_OK, finalmsg);

                    current = current->next;
                }
                // Releasing clients locks.
                current = head;
                while (1) {
                    if (current == NULL) break;
                    retvalue = pthread_mutex_unlock(&(current->handlerequest));
                    if (retvalue != 0) {
                        // Error
                    }
                    current = current->next;
                }
                retvalue = pthread_mutex_unlock(&listmutex);
                if (retvalue != 0) {
                    // Error
                }
                retvalue = pthread_mutex_unlock(&pausemutex);
                if (retvalue != 0) {
                    // Error
                }

                break;
            }case SIGPIPE : {
                // https://stackoverflow.com/questions/108183/how-to-prevent-sigpipes-or-handle-them-properly
                // Nothing, already handled by the single threads.
                ;
                break;
            }default:
                // Error
                break;
        }
    }

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
// This function checks if the current matrix content (global char[][] defined in server.h)
// is legit in accordance with the ALPHABET (also defined in server.h).
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
            // Searching char of matrix in the alphabet.
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
            }
        }
    }

}

// ANCHOR serializeMatrixStr()
// This function allocates on the heap a string.
// It returns a pointer to it.
// The string's length is calculated from NROWS and NCOLS data.
// NCOLS and NROWS are defined in server.h.
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
    The fourth plus 1 is the string terminator (\0).

    Is used to serialize the game matrix var into a string.
    Useful for sending or printing the game matrix.
    
*/
    // Matrix serialized string length.
    const uli MAT_STR_LEN = (NCOL * NROWS * 2) + ((NCOL - 1) * NROWS) + (NROWS) + (1);

    // Allocating the string on the heap.
    char* matrixstring = (char*) malloc(sizeof(char) * MAT_STR_LEN);
    if (matrixstring == NULL) {
        // Error
    }

    // Checking the matrix validation.
    validateMatrix();

    // Initializing the string with a special character, useful for testing and debugging.
    uli counter = 0LU;
    while (counter < MAT_STR_LEN) matrixstring[counter++] = VOID_CHAR;
    // Insert string terminator.
    matrixstring[MAT_STR_LEN] = '\0';

    // Inserting '\n' at the end of each row.
    uli r = 0LU;
    counter = 0LU;
    while (counter < MAT_STR_LEN) {
        // I use the module to understand when end of row in string is reached.
        if (r != 0LU && r % ((NCOL * 2) + (NCOL - 1)) == 0) {
            matrixstring[counter] = '\n';
            r = 0LU;
            counter++;
            continue;
        }
        counter++;
        r++;
    }

    // Inserting letters.
    counter = 0LU;
    uli letters = 0LU;
    // Iterator initialized.
    getMatrixNextIndexes(NULL);
    int matrixnextindexes[2];
    while (counter < MAT_STR_LEN) {
        // End of line reached.
        if (matrixstring[counter] == '\n') {
            letters = 0LU;
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
    counter = 0LU;
    while (counter < MAT_STR_LEN) {
        if (matrixstring[counter] == VOID_CHAR && counter + 1 != MAT_STR_LEN)
            matrixstring[counter] = ' ';
        counter++;
    }

    // Insert string terminator.
    matrixstring[MAT_STR_LEN] = '\0';

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
    }

    // Dictionary already loaded, updating it.
    if (words != NULL) {
        // Cleaning "words".
        for (uli i = 0LU; i < words_len; i++)
            free(words[i]);
        free(words);
        words = NULL;

        words_len = 0LU;

        // Cleaning "words_valid".
        // Clearing also "words_valid" is a good idea to not create a possible insubstantial state.
        if (words_valid != NULL) {
            for (uli i = 0LU; i < words_len; i++)
                free(words_valid[i]);  
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
    }
    
    // Check if the file is regular.
    if(!S_ISREG(s.st_mode)){
            // Error
        }

        // To store file content.
        // Total size, in bytes + 1 for the '\0'. 
        char file[s.st_size + 1];
        char file_copy[s.st_size + 1];

        // Opening the file in readonly mode.
        int fd = open(path, O_RDONLY, NULL);
        if (fd == -1) {
            // Error
        }

        // Reading the file content using a buffer of BUFFER_SIZE length.
        char buffer[BUFFER_SIZE];
        uli counter = 0LU;
        while (1) {
            retvalue = read(fd, buffer, BUFFER_SIZE);
            if (retvalue == -1) {
                // Error
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
            }

            // Copying the word in the new words[i] heap space.
            strcpy(words[counter - 1], str);
            words[counter - 1][strlen(words[counter - 1])] = '\0';

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
    }

    // Realising (if already present) "words_valid".
    if (words_valid != NULL)
        free(words_valid);

    // Allocating space for the new words_valid.
    words_valid = (char**) malloc(sizeof(char*) * words_len);
    if (words_valid == NULL) {
        // Error
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
    // TODO Printing all data in blocks, fix the interleaving prints.
    fprintf(stdout, "Dictionary succesfully validated, founded in the current matrix, these words from dict file:\n");
    
    
    int retvalue;
    // The founded words (valid words) in the dictionary file and now in the current game matrix,
    // will be written in a text file. This will be used to perform tests.
    int fileCurrentValidsWordsFD = -1;
    fileCurrentValidsWordsFD = open(VALID_WORDS_TESTS_FILE_PATH, O_TRUNC | O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);  
    if (fileCurrentValidsWordsFD == -1) {
        // Error
    }

    for (uli i = 0LU; i < words_len; i++) {
        // Valid word.
        if (words_valid[i][0] != '\0') {

            fprintf(stdout, "%s\n", words_valid[i]);

            retvalue = write(fileCurrentValidsWordsFD, words_valid[i], sizeof(char) * strlen(words_valid[i]));
            if (retvalue == -1) {
                // Error
            }
            // Writing new line.
            retvalue = write(fileCurrentValidsWordsFD, "\n", sizeof(char));
            if (retvalue == -1) {
                // Error
            }

            found = 1;

        }
    }
    
    // Closing the text file.
    retvalue = close(fileCurrentValidsWordsFD);
    if (retvalue == -1) {
        // Error
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
// - to simply check if a connected user is registered or not by passing a NULL "name" as arg.
//   It returns 1 if registered, 0 otherwise.
// - to register a new user if the proposed username is not already registered from someone else.
//   It returns -1 if registered succesfully, -2 if the name is already present and c if 
//   the proposed name does not match the ALPHABET of allowed chars (global #define in server.h), 
//   where c is the first invalid char code.
//   So, in these last two cases, no registration will be made.
//   If successful, on the other hand, it fill the "name" struct ClientNode user
//   (by using its pointer) field, by setting it to a new char* heap allocated string, that
//   will contain the new player registered name.
//   It also initialize the "words_validated" struct ClientNode user (by using its pointer) field,
//   by setting it to a new char* heap allocated strings array containing the copied pointers from
//   "words_valid", but only the words that were not previously already accepted for the interested user.
//   In the second use case, it's possible a deadlock situation during the end of the game,
//   in that case, the function resolves the deadlock, saves the message "m" in the struct
//   to process later, and returns -3.
// If name != NULL and an other unexpected error happens, the registerUser() throw a NON CRITICAL ERROR.
// It simply unlock all the acquired mutexes and returns -4, trying to continue.
int registerUser(char* name, struct ClientNode* user, struct Message* m) {

    // Invalid user arg check.
    if (user == NULL) {
        // Error
        handleError(0, 0, 0, 0, "WARNING: Invalid user in registerUser().\nTrying to continue without registering the user.\n");
        return -4; 
    }

    // Only check if the user is registered without registering it when name == NULL is passed.
    if (name == NULL) {
        if (user->name != NULL) return 1;
        return 0;
    }

    // Normalizing in UPPERCASE the player name.
    toLowerOrUpperString(name, 'U');

    // Checking the conformity of the name against the ALPHABET.
    for (unsigned int x = 0; x < strlen(name); x++) {
        char found = 0;
        for (unsigned int i = 0; i < strlen(ALPHABET); i++) {
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
            // Pre-releasing the mutex to fix the deadlock.
            mULock(&(user->handlerequest));
            // To avoiding to re-acquiring immediately.
            // To notify to the caller clientHandler() to not release the already released mutex.
            // Interrupted by end game.
            return -3;
        }else{
            // Error
            user->registerafter = m;
            handleError(0, 0, 0, 0, "WARNING: Error in registerUser(), cannot acquire the \"pausemutex\".\nContinuing without registering.\n");
            // To notify to the caller clientHandler() to not release the already released mutex.
            // Interrupted by end game.
            return -6;
        }
    }

    // TryLock success, owning pausemutex.

    // listmutex CANNOT BE acquired by the signalsThread() thanks to pausemutex, acquiring listmutex,
    // acquiring it before by other threads is not a problem because they always release it.
    mLock(&listmutex);
    // If registerUser() is called, it means at least 1 player should be in the clients list.
    // Remember the clients list contains also the unregistered players, but currently connected to the server.
    if (head == NULL || tail == NULL) {
        // Error
        user->registerafter = m;
        mULock(&(user->handlerequest));
        mULock(&listmutex);
        handleError(0, 0, 0, 0, "WARNING: Error, called registerUser() and the clients list is empty.\nTrying to continue ignoring it.\n");
        return -10;
    }
    // Going through the list.
    // If registerUser() is called, it means at least 1 player should be in the clients list.
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
        mULock(&(user->handlerequest));
        mULock(&listmutex);
        return -2;
    } 

    // If we arrive here the user name is valid and not already registered.

    // Allocating heap space for the user username.
    char* str = (char*) malloc(sizeof(char) * (strlen(name) + 1));
    if (str == NULL) {
        // Error
        handleError(0, 0, 0, 0, "WARNING: Error registering a new user name: %s.\nTrying to continue without registering it.\n");
        mULock(&(user->handlerequest));
        mULock(&listmutex);
        return -8;
    }
    // Copying the new username in heap memory.
    // Terminating string.
    strcpy(str, name);
    str[strlen(str)] = '\0';

    // Linking to the ClientNode relative field.
    user->name = str;

    // Now that the player is registered, i must allocate heap memory and copy
    // the "words_valid" global current file var, to trace
    // the already submitted words.
    user->words_validated = (char**) malloc(sizeof(char*) * words_len);
    if (user->words_validated == NULL) {
        // Error
        mULock(&(user->handlerequest));
        mULock(&listmutex);
        handleError(0, 0, 0, 0, "WARNING: Error in allocating heap memory for \"words_validated\" of a client/player in registerUser().\nTrying to continue without registering it.\n");
        return -12;
    }
    for (unsigned int i = 0; i < words_len; i++) (user->words_validated)[i] = words_valid[i];

    printff(NULL, 0, "Registered user succesfully with the new name %s.\n", user->name);

    mULock(&listmutex);
    mULock(&pausemutex);

    // Registered succesfully.
    return -1;

}

// ANCHOR startGame()
// This function starts a new game.
void startGame(void) {

    // TODO Mutex lock, multiple prints.
    char* banner = bannerCreator(BANNER_LENGTH, BANNER_NSPACES, "NEW GAME STARTED", BANNER_SYMBOL, 0);
    fprintf(stdout, "\n%s\n", banner);
    free(banner);
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
    fprintf(stdout, "Current new matrix:\n%s", mat);
    free(mat);

    // Validate the dictionary with the new game matrix.
    validateDictionary();

    // Setting the new pause timer.
    // The default gameduration is 3 minutes setted in main().
    // The gameduration can also be inserted by a CLI arg.
    // Alarm takes as input seconds, but the user input (gameduration var) is in minutes.
    //alarm(60 * gameduration);
    gameduration = tempteststimeseconds;
    alarm(gameduration);
    fprintf(stdout, "The game duration timer is now setted to %lu minutes.\n", gameduration);

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

    // Pre-allocating heap memory for a new client node.
    // It's just one client, we don't waste a lot of memory.
    struct ClientNode* new = NULL;
    new = (struct ClientNode*) malloc(sizeof(struct ClientNode));
    if (new == NULL) {
        // Error
    }

    clientid++;

    // Waiting for a new client connection.
    int retvalue;
    new->client_address_len = (socklen_t) sizeof(new->client_addr);
    while (1) {
        new->socket_client_fd = accept(socket_server_fd, (struct sockaddr*) (&(new->client_addr)), &(new->client_address_len));
        if (new->socket_client_fd == -1) {
            // Error
        }else
            break;
    }
    fprintf(stdout, "New client succesfully accepted (TMP ID): %lu.\n", clientid);

    // Cannot use PTHREAD_MUTEX_INITIALIZER, because can be used only on static allocated mutexes.
    // Initialization should be performed like this.
    retvalue = pthread_mutex_init(&(new->handlerequest), NULL);
    if (retvalue != 0) {
        // Error
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

    // Adding the client to the list and updating global vars head and tail useful
    // to manage the list.
    // Lock the mutex.
    retvalue = pthread_mutex_lock(&listmutex);
    if (retvalue != 0) {
        // Error
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
        }
        sendMessage(new->socket_client_fd, MSG_ESCI, "Maximum number of clients reached. Disconnecting you... :(\n");
        disconnectClient(new, 0);
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
    }
    fprintf(stdout, "New client thread started succesfully (TMP ID): %lu.\n", clientid);

    retvalue = pthread_mutex_unlock(&listmutex);
    if (retvalue != 0) {
        // Error
    }
    
}

// ANCHOR itoa()
// This function is the "inverse" of atoi().
// It takes a number n (unsigned long int, alias uli) and returns a pointer to an heap
// allocated string containing the chars corresponding to the digits of the number received.
// It's' used to convert values in strings.
char* itoa(uli n) {

    // Below i calculate the number of digits of the received n as input.
    // In this way i can allocate a string of the correct length without wasting space.
    // Based on StackOverflow, but tested and seems to work.

    uli ndigits = n <= 9 ? 1LU : floor (log10 ( (n))) + 1LU;

    // Allocating heap space for the new string of length calculated above.
    char* strint = (char*) malloc(sizeof(char) * ++ndigits); // +1 for '\0'.
    if (strint == NULL) {
        // Error
    }

    // Inserting in the string the number received as input.
    sprintf(strint, "%lu", n);

    // Terminating string.
    strint[ndigits] = '\0';
    
    return strint;

}

// This function is used as support to calculate the times needed.
uli timeCalculator(uli matchorpausetime, char mode) {

    mode = toupper(mode);

    if (mode != 'T' && mode != 'A') {
        // Error
        handleError(0, 0, 0, 0, "WARNING: Error in timeCalculator(), the second arg must be 'T' or 'A'. Trying to continue returning 0.\n");
        return 0LU;
    }

    uli timenow = (uli) time(NULL);
    uli timeremaining;

    timeremaining = timenow - matchorpausetime;

    if (mode == 'T')
        // MSG_TEMPO_PARTITA 'T' Time to end game.
        timeremaining = gameduration - timeremaining;
    else
        // MSG_TEMPO_ATTESA 'A' Time remaining to the start of a new game, pause left time.
        timeremaining = ((uli) PAUSE_DURATION) - timeremaining;

    return timeremaining;

}

// This function simply serialize the current game matrix with serializeMatrixStr()
// and then send it (with sendMessage() function) to the received as input client.
void sendCurrentMatrix(struct ClientNode* client) {

    if (client == NULL) {
        // Error
        handleError(0, 0, 0, 0, "WARNING: Error, sendCurrentMatrix() received a NULL client. Trying to continue without doing nothing...\n");
        return;
    }

    char* mat = serializeMatrixStr();

    sendMessage(client->socket_client_fd, MSG_MATRICE, mat);
    free(mat);
    printff(NULL, 0, "Matrix get request from %s satisfied.\n", client->name);

}

// ANCHOR clientHandler()
// This function will run in a separated dedicated thread and will manage the client's requests.
// Each client will be served by a thread.
// The returned object and the input are void*.
// The output will not be defined (not used, simply returning NULL).
// Input (received void*) must be converted to the correct type struct ClientNode*.
// It rapresents a pointer to the struct ClientNode of the client lists to be managed.
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
    }

    // Casting void* to ClientNode*.
    struct ClientNode* client = (struct ClientNode*) voidclient;

    struct Message* received = NULL;

    client->threadstarted = 1;

    // TODO Block prints interleaved.
    fprintf(stdout, "CONNECTED: I'm a new clientHandler() thread (ID): %lu.\n", (uli) client->thread);
    // Printing the connected client's infos.
    char* strclient = serializeStrClient(client);
    fprintf(stdout, "CONNECTED: %s", strclient);
    free(strclient);
    strclient = NULL;

    int retvalue;

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

        // In this way EVERY REQUEST RECEIVED BEFORE the TIMER, will be processed
        // EXCEPT the registerUser() and disconnectClient(), that will still be 
        // executed automatically after the management of the end of game.
        /*
        
        man pthread_mutex_lock

        [...] If a signal is delivered to a thread waiting for a mutex, upon return from 
        the signal handler the thread shall resume waiting for the mutex as if it was
        not interrupted. [...]
        
        */
        
        // Acquiring the lock.
        while (1) {
            retvalue = pthread_mutex_trylock(&(client->handlerequest));
            if (retvalue != 0) {
                if (retvalue == EBUSY) {
                    usleep(100);
                    client->waiting = 1;
                    continue;
                }else{
                    // Error
                }
            }
            // Lock acquired.
            break;
        }

        // Processing the request.

        // Probably disconnect.
        if (returncode == 0) {
            retvalue = pthread_mutex_unlock(&(client->handlerequest));
            if (retvalue != 0) {
                // Error
            }
            disconnectClient(client, 1);
        }

        // If a player no sends requests an entire game and an entire pause,
        // in the next new game will still have client->actionstoexecute > 0 (if no resetted)!
        // So, it will not fill the queue and the signalsThread() thread
        // will wait forever -> DEADLOCK!
        // For that is important to reset client->actionstoexecute in the signalThread() thread.
 arinza       
        // Executed when the game is paused.
        if (pauseon)
            // First time after end game.
            if (client->actionstoexecute == 0){
                
                // In the below case we have received a message AFTER the end game, but previously
                // of the scoreboard delivery.

                // Specifically, the signalsThread() blocks all the clientHandler() threads,
                // by acquiring all their personal mutexes, enables the pause and releases
                // all the previous acquired mutexes.

                // So, if previously (when signalsThread() was running alone) arrives a
                // message from a client, we are in a special situation in which we are
                // in a transition phase between start pause -> end game.
                // In this case, since the game time is over anyway, these requests
                // (and therefore also those that go to affect the score)
                // will be rejected by alerting the client.    
                // In this particular case the client is notified and the request IGNORED.
                // (since it was received after the deadline it seems fair so).

                // Could you avoid releasing the mutexes by signalHandler() thread
                // and release them only at the end? Of course, this would simplify
                // the whole thing, also avoiding this edge case, however, the philosophy
                // followed is to minimize the holding time of the mutexes in favor of
                // greater speed (due to a greater thread utilization).

                if (received != NULL) {
                    sendMessage(client->socket_client_fd, MSG_IGNORATO, NULL);
                    destroyMessage(&received);
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

        // 2 not present because the signalsThread() thread must execute client->actionstoexecute++,
        // to notify these threads (of the end of scorer() thread) ans so that they can send to clients the
        // final CSV scoreboard.

        // Sending the final scoreboard to the client.
        if (client->actionstoexecute == 3) {
            sendMessage(client->socket_client_fd, MSG_PUNTI_FINALI, scoreboardstr);
            client->actionstoexecute++;
        }

        // Nothing received.
        if (received == NULL) {
            mULock(&(client->handlerequest));
            continue;
        }

        // Processing the new request.
        switch (received->type) {
        
            case MSG_MATRICE: {

                int r = registerUser(NULL, client, received);
                // Not yet authenticated.
                if (r == 0){
                    sendMessage(client->socket_client_fd, MSG_ERR, "You're not authenticated. Please sign up before.\n");
                    printff(NULL, 0, "A user requested the game matrix before the auth.\n");            
                    break;
                }

                if (r == -4) {
                    // Error
                    handleError(0, 0, 0, 0, "WARINNG: Error in the registerUser() in MSG_MATRICE, retrying in a moment...\n");
                    break;
                }
            

                // Pause game check.
                if (!pauseon) {
                    // Sending current game matrix.
                    sendCurrentMatrix(client);
                }else{
                    // The game is paused.
                    // The reply must be:
                    // MSG_TEMPO_ATTESA 'A' Time remaining to the start of a new game, pause left time.

                    // Seconds left to the end of the pause.
                    uli t = timeCalculator(pausetime, MSG_TEMPO_ATTESA);
                    
                    // Casting the number to string to send it in the char* data of the struct Message.
                    char* strint = itoa(t);

                    // Sending MSG_TEMPO_ATTESA.
                    sendMessage(client->socket_client_fd, MSG_TEMPO_ATTESA, strint);
                    printff(NULL, 0, "Matrix get request from name %s converted in time request since the game is paused.\nTime to the next game: %lu.\n", client->name, t);
                    
                    free(strint);

                }

                break; 

            }case MSG_REGISTRA_UTENTE:{
         
                int r = registerUser(NULL, client, received);
                // Already logged in r = 1.
                if (r == 1){
                    sendMessage(client->socket_client_fd, MSG_ERR, "You're already authenticated.\n");
                    printff(NULL, 0, "A user already registered, tried again to register, his current name: %s. Register requested name: %s.\n", client->name, received->data);
                    break;
                }

                if (r == -4) {
                    // Error
                    handleError(0, 0, 0, 0, "WARINNG: Error in the registerUser() in MSG_REGISTRA_UTENTE, retrying in a moment...\n");
                    break;
                }

                // Trying to register the proposed name.
                if (received->data == 0) {
                    // Error
                }
                r = registerUser(received->data, client, received);
                if (r == -4) {
                    // Error
                    handleError(0, 0, 0, 0, "WARINNG: Error in the registerUser() in MSG_REGISTRA_UTENTE, retrying in a moment...\n");
                    break;
                }
                // Interrupted by end game.
                if (r == -3){
                    // Mutex already released.
                    continue;
                } 

                if (r == -6){
                    // Error
                    break;  
                }
                if (r == -10){
                    // Mutex already released.
                    continue;
                } 

                if (r == -2){
                    // Name already present.
                    sendMessage(client->socket_client_fd, MSG_ERR, "Name already present in the game, please chose a different one.\n");
                    printff(NULL, 0, "A user tried registering an already present name: %s.\n", received->data);
                    // Mutex already released.
                    continue;
                }

                if (r > 0) {
                    int l = strlen(ALPHABET);
                    char a[l + 1];
                    strcpy(a, ALPHABET);
                    a[l] = '\0';

                    char stat[] = "The proposed name contains %c, that's not in the alphabet.\nThe alphabet of admitted characters is:\n%s\n";
                    uli total = strlen(stat) + strlen(a) + 2; // +1 for the 'c' char. +1 for the '\0'.
                    char resstr[total];

                    sprintf(resstr, stat, (char)r, a);
                    resstr[total] = '\0';
                    // At least 1 invalid char againist the alphabet in the proposed name.
                    sendMessage(client->socket_client_fd, MSG_ERR, resstr);
                    printff(NULL, 0, "A user tried to register the proposed name: %s. It contains the: %c character, that's invalid againist the alphabet %s.\n", received->data, (char) r, ALPHABET);
                    break;
                }

                if (r == -8 || r == -12) {
                     // Mutex already released.
                    continue;                   
                }

                // Here r must be -1, r == -1.
                // Registered succesfully.
                uli le = strlen(client->name);
                char n[le + 1];
                strcpy(n, client->name);
                n[le] = '\0';
                char sstr[] = "Registered correctly with name: %s. Your ID: %lu.\n";
                char* id = itoa((uli)pthread_self());
                uli total = strlen(n) + strlen(sstr) + strlen(id) + 1;
                char resstr[total];
                sprintf(resstr, sstr, n, (uli) pthread_self());
                resstr[total] = '\0';
                sendMessage(client->socket_client_fd, MSG_OK, resstr);
                printff(NULL, 0, "User registered succesfully, request from name %s satisfied.\n", client->name);

                // pauseon = 0 means the game is ongoing, must send the current game matrix.
                if (!pauseon)
                    sendCurrentMatrix(client);

                // MSG_TEMPO_ATTESA == MSG_TEMPO_RESTANTE (error on text's project)
                // Sending MSG_TEMPO_ATTESA
                char* strint;
                uli t;
                if (pauseon) {
                    // The game is paused.
                    // MSG_TEMPO_ATTESA 'A' Time remaining to the start of a new game, pause left time.
                    
                    // Seconds left to the end of the pause calculation.
                    t = timeCalculator(pausetime, MSG_TEMPO_ATTESA);
                    printff(NULL, 0, "Game in pause during new user signing up. Seconds left to the end of the pause (next game): %lu.\n", t);
                    
                    // Casting the number to string to send it in the char* data of the struct Message.
                    strint = itoa(t);
                    sendMessage(client->socket_client_fd, MSG_TEMPO_ATTESA, strint);

                }else{
                    // The game is ongoing.
                    // MSG_TEMPO_PARTITA 'T' Time to end game.

                    // Seconds left to the end of the game calculation.
                    t = timeCalculator(matchtime, MSG_TEMPO_PARTITA);
                    printff(NULL, 0, "Game going during new user signing up. Seconds left to the end of the game: %lu.\n", t);


                    // Casting the number to string to send it in the char* data of the struct Message.
                    strint = itoa(t);
                    sendMessage(client->socket_client_fd, MSG_TEMPO_PARTITA, strint);
                    
                }

                free(strint);
                break;

            }case MSG_PAROLA: {

                // Not logged in.
                int r = registerUser(NULL, client, received);
                if (r == 0) {
                    sendMessage(client->socket_client_fd, MSG_ERR, "You're not authenticated. Please sign up before.\n");
                    printff(NULL, 0, "A user tried to submit a word before the auth.\n");            
                    break;
                }

                if (r == -4) {
                    // Error
                    handleError(0, 0, 0, 0, "WARINNG: Error in the registerUser() in MSG_PAROLA, retrying in a moment...\n");
                    break;
                }

                // Game in pause.
                if (pauseon) {
                    sendMessage(client->socket_client_fd, MSG_ERR, "Cannot submit words during the game pause. Please, wait a new game.\n");
                    printff(NULL, 0, "User with name %s, submitted word \"%s\" during the game pause. We'll be ignored.\n", client->name, received->data);  
                    break;
                }

                // Submitting the word.
                if (received->data == 0) {
                    // Error
                }
                int p = submitWord(client, received->data);

                if (p == -1) {
                    // -1, Invalid word. Not present in the matrix or WORD_LEN violated.
                    sendMessage(client->socket_client_fd, MSG_ERR, "Invalid word. Try again... Good luck!\n");
                    printff(NULL, 0, "User with name %s, submitted a IN-valid word \"%s\". Assigned 0 points, current total %lu.\n", client->name, received->data, client->points);  
                    break;
                }
                
                // Casting p to long unsigned is safe because the case p == -1
                // is already handled above, so here p >= 0.
                // Note however that sizeof(int) < sizeof(unsigned long int).
                char* strint = itoa((uli) p);
                if (p){
                    // Valid word, never submitted before, p are the achieved points for the word length.
                    sendMessage(client->socket_client_fd, MSG_PUNTI_PAROLA, strint);
                    printff(NULL, 0, "User with name %s, submitted a VALID, and never submitted before, word \"%s\". Assigned %lu points, current total %lu.\n", client->name, received->data, (uli) p, client->points);  
                }else{
                    // Already submitted p == 0.
                    sendMessage(client->socket_client_fd, MSG_PUNTI_PAROLA, strint);
                    printff(NULL, 0, "User with name %s, RE-submitted a valid word \"%s\". Assigned %lu points, current total %lu.\n", client->name, received->data, (uli)p, client->points);  
                }

                free(strint);
                break;

            }case MSG_ESCI : {
                printff(NULL, 0, "Received a disconnection request thread (ID): %lu.\n", (uli)pthread_self());
                destroyMessage(&received);
                mULock(&(client->handlerequest));
                disconnectClient(client, 1);
                // THIS THREAD SHOULD NOW BE DEAD.
            }case MSG_ERR :
            case MSG_OK:
            case MSG_TEMPO_ATTESA:
            case MSG_TEMPO_PARTITA:
            case MSG_PUNTI_PAROLA:
            case MSG_PUNTI_FINALI:
            case MSG_IGNORATO: {
                // Error
                handleError(0, 0, 0, 0, "WARNING: Error, these functions are handled as in exit (sent from this server to the client) not as incoming (from the client to this server).\n");
                break;
            }default:{

                // Error
                // Not recognized message.
                handleError(0, 0, 0, 0, "WARNING: Received an unexpcted message, trying to continue ignoring it...\n");

                break;

            } 

        } // Switch end.

        usleep(100);
        destroyMessage(&received);
        mULock(&(client->handlerequest));

    } // While end.

    return NULL;

}

// This function is used when a player submit a word.
// This function validate a word sent by the client/player.
// It takes as input a player struct ClientNode* pointer, and a char*
// rapresenting the word submitted to validate, NULL terminated.
// It returns:
// - The points added to the player, if the word is correct (founded both in dictionary file 
 // and in the current game matrix) and never submitted before.
// - 0, if the word is correct (founded both in dictionary file and in the current game 
// matrix) but ALREADY submitted (from the interested player) before in the current game.
// - -1, if the word is not correct (not founded) in the current game matrix, but
// could be present in the dictionary (or if present in the dictionary but not in the
// current game matrix).
int submitWord(struct ClientNode* player, char* word) {

    if (word == NULL || player == NULL) {
        // Error
        handleError(0, 0, 0, 0, "WARNING: Error in submitWord(), NULL player or word received. Trying to continue refusing the request.\n");
        return -1;
    }

    // Normalizing the word to UPPERCASE.
    toLowerOrUpperString(word, 'U');

    // i is the index of the searched word in the "words_valid" var.
    // "words_valid" mantain only the words present BOTH in the dictionary
    // AND in the current game matrix.
    // For more info see validateDictionary() function.
    int i = validateWord(word);

    // If -1 the word has not been found in the "words_valid" var.
    // This means that the searched word is not present in the current game matrix.
    // Note that it may still be in the dictionary, however.
    if (i == -1) return -1;

    // Below I use the i "words_valid" index to access to the "player->words_validated".
    // I can do this since are aligned on the words contained.

    // Already submitted word, 0 points.
    if (player->words_validated[i][0] == '\0') return 0;
    else {
        // Calculating the points to return.
        uli p = strlen(word);
        // Adding the points to the player's total.
        player->points += p;
        // 'Qu' correction, in strlen 'Q' and 'u' are counted as 2.
        // But we only want it to be worth 1 point.
        uli qu = 0L;
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

// This function returns the index of a searched word in the dictionary, so it's the index 
// of the searched word in the "words" strings array.
// (and so the same as "words_valid" because the array are aligned).
// If the word is not valid (not present in the dictionary, so not in "words") it returns -1.
// Takes as input a pointer to the word (string) to search.
int validateWord(char* word) {

    // Validating the matrix.
    validateMatrix();

    // Dictionary not previous loaded.
    if (words == NULL) {
        // Error
        handleError(0, 1, 0, 0, "Error, validateWord() cannot continue if loadDictionary() hasn't been called before.\n");
    }

    // Dictionary not previous validated.
    if (words_valid == NULL) {
        // Error
        handleError(0, 1, 0, 0, "Error, validateWord() cannot continue if loadDictionary() has been called, but the validateDictionary() after not.\n");
    }

    // Empty word.
    if (word == NULL) {
        // Error
        handleError(0, 0, 0, 0, "WARNING: Error, validateWord() received an empty word. Trying to continue returning -1.\n");
        return -1;
    }

    // Word length.
    uli s = strlen(word);
    // 'Qu' correction.
    uli cs = s;
    for (uli i = 0LU; i < cs; i++)
        if (i + 1 != cs && word[i] == 'Q' && word[i + 1] == 'U') s--;
    // If is set a constraint (WORD_LEN > 0) on the minimum word length, it is applied.
    if (WORD_LEN > 0 && cs < WORD_LEN) return -1;

    // The client submitted word is totally converted
    // to the UPPERCASE version, format in which the characters in the game matrix, 
    // the words in the dictionary file and the ALPHABET used to generate random matrices,
    // are loaded into memory.
    // There will be no difference between a client input like "home"
    // or "HoMe" or "HOME".
    // Will all be accepted if the word is present in the dictionary file and the
    // current game matrix.
    toLowerOrUpperString(word, 'U');

    // Searching the word in the dictionary containing only the the current game matrix words.
    for (unsigned int i = 0; i < words_len; i++)
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
    }

    struct ClientNode* client = *clienttodestroy;

    // Invalid client.
    if (client == NULL) {
        // Error
    }

    // Printing the disconnecting client's infos.
    char* strclient = serializeStrClient(client);
    fprintf(stdout, "DISCONNECTION: %s", strclient);
    free(strclient);
    strclient = NULL;

    // Notifying the signalsThread() thread of this client's disconnection.
    client->toexit = 1;

    // WHY USING GOTO THAT CAUSES SPAGHETTI CODE?!?!
    // Because this function could recursively recall itself so many times
    // before signalsThread() releases the mutexes needed.
    // This could cause stackoverflow, but fortunally we don't need the stack of
    // the previous call, so in this case the goto is perfect to jump at the start
    // of this function.

disconnect_restart: {
    client->waiting = 1;
    retvalue = pthread_mutex_trylock(&pausemutex);
    if (retvalue != 0) {
        if (retvalue == EBUSY) {
            // PAUSE IS ON!          
            // To avoiding to re-acquiring immediately.
            usleep(100);
            goto disconnect_restart;
        }else{
            // Error
        }
    }
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
        }
        char found = 0;
        // Empty list (impossible if a client is requested to be disconnected).
        if (head == NULL || tail == NULL) {
            // Error
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
            if (current == NULL){
                // Error
                // This is an error. The disconnected client is not in the list!?
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
            }
            retvalue = pthread_mutex_unlock(&listmutex);
            if (retvalue != 0) {
                // Error
            }
            return;
        }
        nclientsconnected--;
        retvalue = pthread_mutex_unlock(&pausemutex);
        if (retvalue != 0) {
            // Error
        }
        retvalue = pthread_mutex_unlock(&listmutex);
        if (retvalue != 0) {
            // Error
        }

        // Cleaning the client object.
        client->client_address_len = 0;
        client->next = NULL;
        retvalue = pthread_mutex_destroy(&(client->handlerequest));
        if (retvalue != 0) {
            // Error
        }
        client->points = 0LU;
        if (client->words_validated) free(client->words_validated);
        client->words_validated = NULL;
        if (client->name) free(client->name);
        client->name = NULL;
        if (client->registerafter) free(client->registerafter);
        client->registerafter = NULL;
        client->actionstoexecute = 0;
        client->receivedsignal = 0;
        client->waiting = 0;
        client->toexit = 0;
        client->filledqueue = 0;

        retvalue = close(client->socket_client_fd);
        if (retvalue != 0){
            // Error
        }
        client->socket_client_fd = -1;

        uli tmpt = client->threadstarted == 0 ? 0LU : (uli) client->thread;

        free(client);

        *clienttodestroy = NULL;
  
        fprintf(stdout, "DISCONNECTION: %lu (ID) client has disconnected succesfully.\n", tmpt);

        if (terminatethread) pthread_exit(NULL);
        else return;

    }


// This function is the SIGUSR1 signal handler.
// It executed by every clientHandler() thread, when received the signal SIGUSR1 from signalsThread(),
// which corresponds to the end of the game.
void endGame(int signum) {

    // This is used only to interrupt the read() with EINTR in errno.
    // ANCHOR endGame()
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
        // printf(NULL) -> SISGEV so cannot trying to continue returning NULL.
    }

    // Preparing the string format.
    char st[] = "Name: %s - IP: %s - Port: %lu - Points %lu - Thread ID: %lu.\n";

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

    // Getting IP.
    char buff[INET_ADDRSTRLEN] = {0}; 
    const char* s = inet_ntop(AF_INET, &(c->client_addr.sin_addr), buff, c->client_address_len);
    if (s == NULL){
        // Error
    }
    uli iplength = strlen(s);

    // Calculating points length (as string).
    char* pointsstr = itoa(c->points);
    uli pointslen = strlen(pointsstr);
    free(pointsstr);

    // Calculating Thread ID length (as string).
    char* threadidstr;
    if (c->threadstarted) threadidstr = itoa((uli) c->thread);
    else threadidstr = itoa(0LU);
    uli threadidstrlen = strlen(threadidstr);
    free(threadidstr);

    // Allocating the needed heap memory to store the string.
    // +1 for '\0'.
    uli totallength =  strlen(st) + namelen + iplength + portlen + pointslen + threadidstrlen + 1;
    char* rs = (char*) malloc((totallength) * sizeof(char));
    if (rs == NULL) {
        // Error
    }

    // Filling the string with data.
    sprintf(rs, st, name, buff, port, c->points, c->threadstarted ? (uli) c->thread : 0LU);

    return rs;

}

// This function should be called at the end of the scorer() thread.
// It creates the end game final scoreboard string.
// The format is CSV with "playername,playerpoints".
// The functions allocate the string on the heap and set the char* "scoreboardstr"
// global var to it.
// It takes as input an array of Queue* and its length (as second arg).
void createScoreboard(struct Queue** array, int arraylength) {

    // Input checks.
    if (arraylength < 0 || array == NULL || *array == NULL) {
        // Error
        handleError(0, 0, 0, 0, "WARNING: Error in createScoreboard(), received wrong args. Trying to continue without creating the scoreboard.\n");
        return;
    }

    // Calculating string length and allocating the corresponding heap space.
    uli totallength = 0LU;
    for (unsigned int i = 0; i < arraylength; i++) totallength += strlen(array[i]->message->data);
    totallength += arraylength - 1; // For ',' after points (next couple "playername,playerpoints").
    totallength++; // For '\0'.
    scoreboardstr = (char*) malloc(sizeof(char) * totallength);
    if (scoreboardstr == NULL) {
        // Error
        handleError(0, 0, 0, 0, "WARNING: Error in createScoreboard(), in malloc(). Trying to continue without creating the scoreboard.\n");
        return;
    }

    // Copying all data from the Queue* [] (array) by accessing to each "message->data" field.
    uli counter = 0LU;
    for (unsigned int i = 0; i < arraylength; i++) {
        char* c = array[i]->message->data;
        while (c[0] != '\0'){
            scoreboardstr[counter++] = c[0];
            c++;
        } 
        scoreboardstr[counter++] = ',';
    }
    // Inserting string terminator.
    scoreboardstr[counter - 1] = '\0';

}

// ANCHOR gameEndQueue()
// This function create (and add element to) a heap allocated queue.
// Each struct Queue element contains a struct ClientNode* object,
// a struct Message* object and a pointer to the next queue element.
// It's called by each clientHandler() thread at end of game.
// Is possible to use the queue by accessing to "headq" and "tailq" global struct Queue*.
// It takes as input a struct ClientNode* pointer rapresenting the client to add to the queue.
// The struct Message will contain in the "data" field a CSV string message with the
// format "playername,playerpoints".
// It's required by the project's text.
void gameEndQueue(struct ClientNode* e) {

    // Invalid client.
    if (e == NULL) {
        // Error
        handleError(0, 1, 0, 0, "Error in gameEndQueue(), NULL client received.\n");
        return;
    }

    // Creating and filling message object.

    // Allocating space for the message on the heap.
    struct Message* m = (struct Message*) malloc(sizeof(struct Message));
    if (m == NULL) {
        // Error
        handleError(0, 1, 0, 0, "Error in gameEndQueue(), in malloc() of message.\n");
        return;
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
    uli length = e->name == NULL ? strlen(NO_NAME) + plength : strlen(e->name) + plength;
    m->length = length;

    // Allocating string heap space.
    m->data = (char*) malloc(sizeof(char) * m->length);
    if (m->data == NULL) {
        // Error
        handleError(0, 1, 0, 0, "Error in gameEndQueue(), in malloc() of message->data.\n");
        return;
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

    uli counter2 = 0LU;
    while(1) {
        m->data[counter] = p[counter2];
        if (p[counter2] == '\0') break;
        counter++;
        counter2++;
    }

    free(p);

    // Creating and filling the new queue element.
    struct Queue* new;
    new = (struct Queue*) malloc(sizeof(struct Queue));
    if (new == NULL) {
        // Error
        handleError(0, 1, 0, 0, "Error in gameEndQueue(), in malloc() of struct Queue.\n");
    }
    // Setup the new queue element.
    new->next = NULL;
    new->client = e;
    new->message = m;

    // A simply mutex used only to sync with others threads in the queue.
    mLock(&queuemutex);

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
 //   char* namestr = csvNamePoints(m, 0);
//    char* pointsstr = csvNamePoints(m, 1);
char namestr[] = "PEIRO";
char pointsstr []= "50";

    printff(NULL, 0, "PUSHED: New object pushed to the tail queue.\nPUSHED: Player's name: %s. Player's points: %s. Thread (ID): %lu.\n", namestr, pointsstr, (uli) pthread_self());
  //  free(namestr);

  //  free(pointsstr);

    mULock(&queuemutex);

}


/*

##########################              EXAMPLE             ##########################

Let's assume that loadDictionary("/path/to/file.txt") has been called, now we will
have "char** words" var, filled with all the words present in the dictionary
file "/path/to/file.txt" (assuming a word for line terminated by \n or by \r\n).

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
Also, let's assume there is not the constraint on the word length.

Now let's call validateDictionary(); At the end we will have "char** words_valid" global var
filled with:
char** words_valid -> words_valid[0] (char*) [0x16d22726c same pointer of words[0]] -> "hello\0"
                  -> words_valid[1] (char*) [0x16d227275 same pointer of words[1] + 3 bytes] -> "\0"
                  -> words_valid[2] (char*) [0x16d227276 same pointer of words[2]] -> "mum\0"

The word "dog" is not present because it is in the dictionary file, and so in the "char** words",
but NOT in the current game matrix. Instead "hello\0" and "mum\0" are present because were
founded BOTH in the dictionary file (char** words) and the current game matrix (look at
the lowercase letters above).

The "dog\0" word was not deleted, we simply updated the copied pointer, incrementing it.

Note that we are operating by exploiting the power of pointers, with their arithmetic,
without having two copies of the strings in memory, but only two arrays with their pointers (char*).

*/

/*

 ########   SOME INFORMATIONS ON SYNCHRONIZATION BETWEEN THREADS AND SIGNAL MANAGEMENT  ########
    
sigwait() handles all SIGINT and SIGALRM signals running in a dedicated thread
called signalsThread(), started in the main after blocking them.
If more signals arrive, the first one to arrive is put in pending, the others lost.
Therefore, I cannot be interrupted by a new signal (of these) while handling the previous one.
When a game is over, the alarm() (setted on start of new game) trigger an SIGALRM signal caugth by signalsThread().
So, the signalsThread() waits all threads to complete the current response in processing, then
it acquires their mutex (each clientHandler() has its own mutex that release after completing the client response).
Then, we use pthread_kill() to inform each clientHandler() thread that the game is over
with a SIGUSR1 signal.
In this way, all threads (both those blocked on the read and those blocked immediately afterwards
waiting for the mutex that allows them to continue processing the received request)
are "suspended" on their own mutex.
Specifically, threads locked on reads are interrupted and the read will fail returning EINTR in errno.
In the case of a partial received request (e.g. read interrupted on receiving the
unsigned int length of the data field, but it has already read the message type)
we inform the client of that, we ignore this request, and we clear all received socket
stream (of the interested client) to avoid subsequent misaligned readings of the message fields.
Now we can enable safety the pause (pauseon global var), because we will be sure that
no thread can perform unsafe multithreaded actions.
Then we release all clientHandler() mutexes (for each client, its own).
Now all the clients will acquire their locks and will detect the end of game, so they
will fill the end-game queue, which the thread scorer will subsequently take care of as
required by the project text. Finally now all the threads can receive and response to all
the clients requests considering the variable “pauseon” the fact that the game is paused.

*/


// Shared server cross files vars and libs.
#include "server.h"

// Current file vars and libs.
#include <getopt.h> // Used to check optionals args.
#define USAGE_MSG "Invalid args. Usage: ./%s server_ip server_port [--matrices matrices_filepath] [--duration game_duration_in_minutes] [--seed rnd_seed] [--dic dictionary_filepath].\n" // Message to print when the user inserts wrong args.
#define DEFAULT_DICT "./Data/Dicts/dictionary_ita.txt" // Default dictionary file path, used when --dic is not present.

int main(int argc, char** argv) {

    // Printing start banner.
    char* banner = bannerCreator(BANNER_LENGTH, BANNER_NSPACES, "SETUP!", BANNER_SYMBOL, 0);
    fprintf(stdout, "\n\n##################\n#     SERVER     #\n##################\n\n");
    fprintf(stdout, "%s\n", banner);
    free(banner);
    banner = NULL;

    // Initializing local vars.
    int retvalue = 0; // To check system calls result (succes or failure).
    uli seed = 0LU; // Random seed.
    struct sigaction sigusr1; // SIGUSR1 sigaction struct, will be used to handle the game end using a SIGUSR1 signal.

    // Initializing shared server cross vars.
    socket_server_fd = -1;
    gameduration = 0LU;
    usematrixfile = 0;
    matpath = NULL;

    // Shared/Common CLIENT & SERVER cross files vars and libs initialization.
    mainthread = pthread_self();
    setupfinished = 0;
    // Cannot use PTHREAD_MUTEX_INITIALIZER, because can be used only on static allocated mutexes.
    // Initialization should be performed like this.
    retvalue = pthread_mutex_init(&setupmutex, NULL);
    if (retvalue != 0) {
        // Error
    }
    retvalue = pthread_mutex_init(&printmutex, NULL);
    if (retvalue != 0) {
        // Error
    }

    fprintf(stdout, "I'm the main thread (ID): %lu.\n", (uli) mainthread);
    pthread_setname_np(mainthread, "MainThread");

    // Creating a mask, that will block the SIGINT, SIGALRM and SIGPIPE signals for all 
    // threads except the dedicated thread signalsThread().
    // Important to do it as soon as possible to be able to handle appropriately the signals.
    // Not to be caught unprepared.
    sigemptyset(&signalmask);
    sigaddset(&signalmask, SIGINT);
    sigaddset(&signalmask, SIGALRM);
    sigaddset(&signalmask, SIGPIPE);

    // Enabling the signals' mask.
    retvalue = pthread_sigmask(SIG_BLOCK, &signalmask, NULL);
    if (retvalue != 0) {
        // Error
    }
    fprintf(stdout, "Threads signals mask enabled correctly.\n");
  
    // SIGUSR1 will NOT be blocked and NOT handled by the signalsThread() thread.
    // Setting the endGame() SIGUSR1 handler.
    // This signal will be sent by the signalsThread() thread to each clientHandler() thread.
    // Is used to notify the game end.
    sigusr1.sa_flags = 0;
    sigusr1.sa_handler = endGame;  
    retvalue = sigaction(SIGUSR1, &sigusr1, NULL);
    if (retvalue == -1) {
        // Error
    }           
    siginterrupt(SIGUSR1, 1);
    fprintf(stdout, "SIGUSR1 signal handler registered correctly.\n");

    // Any newly created threads INHERIT the signal mask with SIGALRM, SIGINT, SIGPIPE signals blocked. 

    // Creating the pthread that will handle ALL AND EXCLUSIVELY SIGINT, SIGALRM and SIGPIPE signals
    // by waiting with sigwait() forever in loop.
    retvalue = pthread_create(&signalsthread, NULL, signalsThread, NULL);
    if (retvalue != 0) {
        // Error
    }
    fprintf(stdout, "Signals registered and pthread signals handler started succesfully.\n");

    // Check number of args.
    if (argc < 3 || argc > 11 || ((argc > 3) && (argc != 5 && argc != 7 && argc != 9 && argc != 11))) {
        // Error
    }

    // Parsing port.
    uli port = strtoul(argv[2], NULL, 10);
    if (port > 65535LU) {
        // Error
    }
    server_addr.sin_port = htons(port);

    // Socket type.
    server_addr.sin_family = AF_INET;
    
    // Parsing IP.
    retvalue = parseIP(argv[1], &server_addr);
    if (retvalue != 1) {
        // Error
    }

    // Checking optionals args.
    char* filemath = NULL;
    char* filedict = NULL;
    for (uli i = 3LU; i < argc; i += 2LU) {
        // Lowering input.
        toLowerOrUpperString(argv[i], 'L');
        if (strcmp(argv[i], "--matrices") == 0 || strcmp(argv[i], "--matrici") == 0 || strcmp(argv[i], "--mat") == 0)
            filemath = argv[i + 1];
        else if (strcmp(argv[i], "--duration") == 0 || strcmp(argv[i], "--durata") == 0) {
            gameduration = (uli) strtoul(argv[i + 1], NULL, 10);
        }else if (strcmp(argv[i], "--seed") == 0){
            seed = (uli) strtoul(argv[i + 1], NULL, 10);
        }else if (strcmp(argv[i], "--dic") == 0 || strcmp(argv[i], "--diz") == 0){
            filedict = argv[i + 1];
        }else{
            // Error
        }
    }

    // Setting default args and printing infos.
    char seedpresent = 0;
    if (filemath != NULL) {
        matpath = filemath;
        usematrixfile = 1;
        fprintf(stdout, "Trying to use matrix file at: %s.\n", matpath);
    }else
        fprintf(stdout, "Using RANDOM matrices.\n");
    if (gameduration != 0LU)
        fprintf(stdout, "Using INSERTED match time duration %lu minutes.\n", gameduration);
    else {
        gameduration = 3LU;
        fprintf(stdout, "Using DEFAULT match time duration %lu minutes.\n", gameduration);
    }
    if (seed != 0LU) {
        seedpresent = 1;
        fprintf(stdout, "Using INSERTED seed %lu.\n", seed);
    }else {
        seed = RAND_SEED;
        fprintf(stdout, "Using DEFAULT seed %lu.\n", seed);
    }

    // Further checks on optional args added after the suggestion given by Prof.
    // Using getopt_long.
    // For infos:
    // https://man7.org/linux/man-pages/man3/getopt.3.html
    // Starting parsing from 3, because 0 = Program Name, 1 = IP, 2 = Port, 3 = ...
    optind = 3;
    // Suppressing opt library error messages, will be used mine.
    opterr = 0;
    // c will be the character read.
    int c; 
    while (1) {
        int option_index = 0;
        // Allowed args.
        struct option long_options[] = {
            {"matrici",     required_argument, 0,  0 },
            {"diz",  required_argument, 0,  0 },
            {"seed",  required_argument, 0, 0},
            {"durata",    required_argument, 0,  0 },
            {"matrices",     required_argument, 0,  0 },
            {"dic",  required_argument, 0,  0 },
            {"duration",    required_argument, 0,  0 },
            // This last is needed, otherwise SIGSEGV!
            {0,         0,                 0,  0 }
        };
        // 3 arg is a string of allowed option character after -, in this case empty.
        c = getopt_long(argc, argv, "", long_options, &option_index);
        // Terminated reading args.
        if (c == -1)
            break;
        // Parsing character read.
        switch (c) {
            case 0:
            // long_options[option_index].name // Is the arg name.
            // optarg // Is the arg value.

            // All OK, all other controls have been performed previously.
            break;

            case '?':  // Unrecognized or not allowed character parsed.
            default:
                // Error
                ;                
        }
    }

    if (optind < argc) {
        // Error
        // printff(NULL, 1, "Error, unrecognized args: ");
        // while (optind < argc) printff(NULL, 1, "%s ", argv[optind++]);
        // printff(NULL, 1, "\n");
        // printff(NULL, 1, USAGE_MSG, argv[0]);
    }

    if (usematrixfile && seedpresent) {
        // Error
    }

    fprintf(stdout, "The args seems to be ok...\n");

    // Setting rand seed.
    srand(seed);
    fprintf(stdout, "Initialized random seed %lu.\n", seed);

    // Loading words dictionary in memory.
    if (filedict != NULL)
        fprintf(stdout, "Trying to use CUSTOM dictionary file at: %s.\n", filedict);
    else{
        filedict = DEFAULT_DICT;
        fprintf(stdout, "Trying to use DEFAULT dictionary file at %s.\n", filedict);
    }
    loadDictionary(filedict);
 
    // Initializing game matrix.
    // Initializing the matrix with a special symbol (VOID_CHAR), useful for testing and debugging.
    for (uli i = 0LU; i < NROWS; i++)
        for (uli j = 0LU; j < NCOL; j++) 
            matrix[i][j] = VOID_CHAR;
    fprintf(stdout, "Game matrix succesfully initialized.\n");

    // Creating socket.
    // READ man socket, there are useful infos.
    socket_server_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
    if (socket_server_fd == -1) {
        // Error
    }
    fprintf(stdout, "Server socket created.\n");
    
    // Binding socket.
    retvalue = bind(socket_server_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
    if (retvalue == -1){
        // Error
    }
    fprintf(stdout, "Binding completed.\n");

    // Listening for incoming connections.
    retvalue = listen(socket_server_fd, SOMAXCONN);
    if (retvalue == -1) {
        // Error
    }
    fprintf(stdout, "Listening...\n");

    // Waiting for the setup of other threads.
    char toexit = 0;
    while (1) {
        retvalue = pthread_mutex_lock(&setupmutex);
        if (retvalue != 0) {
            // Error
        }
        if (setupfinished == 1) toexit = 1;
        retvalue = pthread_mutex_unlock(&setupmutex);
        if (retvalue != 0) {
            // Error
        }
        if (toexit) break;
        // To avoid instant reacquiring.
    }

    // Printing end banner.
    banner = bannerCreator(BANNER_LENGTH, BANNER_NSPACES, "SETUP!", BANNER_SYMBOL, 1);
    fprintf(stdout, "%s\n", banner);
    free(banner);
    banner = NULL;

    fflush(stdout);

    // Starting the first game.
    startGame();

    // Updating clients (the first time not needed but done for clarity).
    updateClients();

    // Accepting new clients.
    while(1) acceptClient();

    // This never executed, but for clarity.
    exit(EXIT_SUCCESS);

    return 0;
    
}



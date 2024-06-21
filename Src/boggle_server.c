// Shared server cross files vars and libs.
#include "server.h"

// Current file vars and libs.
#include <getopt.h> // Used to check optionals args.
#define USAGE_MSG "Invalid args. Usage: ./%s server_ip server_port [--matrices matrices_filepath] [--duration game_duration_in_minutes] [--seed rnd_seed] [--dic dictionary_filepath].\n" // Message to print when the user insert wrong args.
#define DEFAULT_DICT "../Data/dictionary_ita.txt" // Default dict used when --dic is not present.

int main(int argc, char** argv) {

    // Printing banner.
    // Normal printf because the printmutex used in printff() is not initialized yet.
    printf("\n\n##################\n#     SERVER     #\n##################\n\n");
    printf("################################ SETUP ################################\n");

    // Initializing local vars.
    int retvalue = 0; // To check system calls result (succes or failure).
    uli seed = 0LU; // Random seed.
    struct sigaction sigusr1; // SIGUSR1 sigaction struct, will be used to handle the game end.

    // Initializing shared server cross vars.
    socket_server_fd = 0;
    gameduration = 0LU;
    usematrixfile = 0;
    matpath = NULL;
    threadsignalreceivedglobal = NULL;

    // Shared/Common CLIENT & SERVER cross files vars and libs initialization.
    mainthread = pthread_self();
    testmode = 0;
    // PTHREAD_MUTEX_INITIALIZER only available with statically allocated variables.
    // In this case i must use pthread_mutex_init().
    retvalue = pthread_mutex_init(&mutexprint, NULL);
    if (retvalue != 0) {
        // Error
        handleError(0, 1, 0, 0, "Error in printmutex initializing.\n");
    }

    printff(NULL, 0, "I'm the main thread (ID): %lu.\n", (uli)pthread_self());
    // To setup the thread destructor.
    threadSetup();

    // Creating a mask, that will block the SIGINT, SIGALRM and SIGPIPE signals for all 
    // threads except the dedicated thread signalsThread().
    // Important to do it as soon as possible.
    // Not to be caught unprepared.
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGINT);
    sigaddset(&signal_mask, SIGALRM);
    sigaddset(&signal_mask, SIGPIPE);

    // Registering exit function for main.
    retvalue = atexit(atExit);
    if (retvalue != 0) {
        // Error
        handleError(1, 1, 0, 0, "Error in registering exit cleanupper with atexit() in main function.\n");
    }
    printff(NULL, 0, "Exit safe function (cleanup for the main with atexit()) registered correctly.\n");

    // Enabling the mask.
    retvalue = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
    if (retvalue != 0) {
        // Error
        handleError(0, 1, 0, 0, "Error in setting the pthread signals mask.\n");
    }
    printff(NULL, 0, "Threads signals mask enabled correctly.\n");
  
    // SIGUSR1 will NOT be blocked.
    // Setting the endGame() handler.
    // This signal will be send by the signalsThread() thread to each clientHandler() thread.
    // Is used to notify the game end.
    sigusr1.sa_flags = 0;
    sigusr1.sa_handler = endGame;  
    retvalue = sigaction(SIGUSR1, &sigusr1, NULL);
    if (retvalue == -1) {
        // Error
        // errno
        handleError(1, 1, 0, 0, "Error in setting SIGUSR1 signal handler.\n");
    }           
    printff(NULL, 0, "SIGUSR1 signal handler registered correctly.\n");

    // Any newly created threads INHERIT the signal mask with SIGALRM, SIGINT, SIGPIPE signals blocked. 

    // Creating the thread that will handle ALL AND EXCLUSIVELY SIGINT, SIGALRM and SIGPIPE signals
    // by waiting with sigwait() forever in loop.
    retvalue = pthread_create(&sig_thr_id, NULL, signalsThread, NULL);
    if (retvalue != 0) {
        // Error
        handleError(0, 1, 0, 0, "Error in creating the pthread signals handler.\n");
    }
    printff(NULL, 0, "Signals registered and pthread handler started succesfully.\n");

    // Check number of args.
    if (argc < 3 || argc > 11 || ((argc > 3) && (argc != 5 && argc != 7 && argc != 9 && argc != 11))) {
        // Error
        handleError(0, 1, 0, 0, USAGE_MSG, argv[0]);
    }

    // Parsing port.
    uli port = strtoul(argv[2], NULL, 10);
    if (port > 65535LU) {
        // Error
        handleError(0, 1, 0, 0, "Invalid port %lu. It should be less than 65535.\n", port);
    }
    server_addr.sin_port = htons(port);

    server_addr.sin_family = AF_INET;
    
    // Parsing IP.
    retvalue = parseIP(argv[1], &server_addr);
    if (retvalue != 1) {
        // Error
        handleError(0, 1, 0, 0, "Invalid IP: %s.\n", argv[1]);
    }

    // Checking optionals args.
    char* filemath = NULL;
    char* filedict = NULL;
    for (unsigned int i = 3; i < argc; i += 2) {
        // Lowering input.
        toLowerOrUpperString(argv[i], 'L');
        if (strcmp(argv[i], "--matrices") == 0)
            filemath = argv[i + 1];
        else if (strcmp(argv[i], "--duration") == 0) {
            gameduration = (uli) strtoul(argv[i + 1], NULL, 10);
        }else if (strcmp(argv[i], "--seed") == 0){
            seed = (uli) strtoul(argv[i + 1], NULL, 10);
        }else if (strcmp(argv[i], "--dic") == 0){
            filedict = argv[i + 1];
        }else{
            // Error
            handleError(0, 1, 0, 0, USAGE_MSG, argv[0]);
        }
    }
    printff(NULL, 0, "Starting server on IP: %s and port: %lu.\n", argv[1], port);

    // Setting default args and printing infos.
    if (filemath != NULL) {
        matpath = filemath;
        usematrixfile = 1;
        printff(NULL, 0, "Trying to use matrix file at: %s.\n", matpath);
    }else
        printff(NULL, 0, "Using RANDOM matrices.\n");
    if (gameduration != 0LU)
        printff(NULL, 0, "Using INSERTED match time duration %lu minutes.\n", gameduration);
    else {
        gameduration = 3LU;
        printff(NULL, 0, "Using DEFAULT match time duration %lu minutes.\n", gameduration);
    }
    if (seed != 0LU)
        printff(NULL, 0, "Using INSERTED seed %lu.\n", seed);
    else {
        seed = 42LU;
        printff(NULL, 0, "Using DEFAULT seed %lu.\n", seed);
    }

    // Further checks on optional args added after the suggestion given by Prof.
    // getopt_long
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
        struct option long_options[] = {
            {"matrici",     required_argument, 0,  0 },
            {"diz",  required_argument, 0,  0 },
            {"seed",  required_argument, 0, 0},
            {"durata",    required_argument, 0,  0 },
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
                handleError(0, 1, 0, 0, "Error while parsing args with getopt_long().\n");
        }
    }

    if (optind < argc) {
        // Error
        mLock(&mutexprint);
        printff(NULL, 1, "Error, unrecognized args: ");
        while (optind < argc) printff(NULL, 1, "%s ", argv[optind++]);
        printff(NULL, 1, "\n");
        printff(NULL, 1, USAGE_MSG, argv[0]);
        mULock(&mutexprint);
        handleError(0, 1, 0, 0, "Error, unrecognized args.\n");
    }

    printff(NULL, 0, "The args seems to be ok...\n");

    // Setting rand seed.
    srand(seed);
    printff(NULL, 0, "Initialized random seed %lu.\n", seed);

    // Loading words dictionary in memory.
    if (filedict != NULL)
        printff(NULL, 0, "Trying to use dictionary file at: %s.\n", filedict);
    else{
        filedict = DEFAULT_DICT;
        printff(NULL, 0, "Trying to use DEFAULT dictionary file at %s.\n", filedict);
    }
    loadDictionary(filedict);
 
    // Initializing game matrix.
    initMatrix();

    // Creating socket.
    // READ man socket, there are useful infos.
    socket_server_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
    if (socket_server_fd == -1) {
        // Error
        handleError(0, 1, 0, 0, "Error in creating the server socket.\n");
    }
    printff(NULL, 0, "Server socket created.\n");
    
    // Binding socket.
    retvalue = bind(socket_server_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
    if (retvalue == -1){
        // Error
        handleError(0, 1, 0, 0, "Error in binding.\n");
    }
    printff(NULL, 0, "Binding completed.\n");

    // Listening for incoming connections.
    retvalue = listen(socket_server_fd, SOMAXCONN);
    if (retvalue == -1) {
        // Error
        // errno
        handleError(1, 1, 0, 0, "Error in listening.\n");
    }
    printff(NULL, 0, "Listening...\n");
    printff(NULL, 0, "################################ END SETUP ################################\n");

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



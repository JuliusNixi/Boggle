// Shared server cross files vars and libs.
#include "server.h"

// Current file vars and libs.
#include <getopt.h> // Used to check optionals args.
#define USAGE_MSG "Invalid args. Usage: ./%s server_ip server_port [--matrices matrices_filepath] [--duration game_duration] [--seed rnd_seed] [--dic dictionary].\n" // Message to print when the user insert wrong args.
#define DEFAULT_DICT "../Data/dictionary_ita.txt" // Default dict used when --dic is not present.

int main(int argc, char** argv) {

    // Initializing local vars.
    int retvalue = 0; // To check system calls result (succes or failure).
    unsigned long int seed = 0U; // Random seed.

    // Initializing shared server cross vars.
    socket_server_fd = 0;
    gameduration = 0LU;
    usematrixfile = 0;
    matpath = NULL;

    // Shared/Common CLIENT & SERVER cross files vars and libs initialization.
    mainthread = pthread_self();

    // Printing banner.
    printff(NULL, "\n\n##################\n#     SERVER     #\n##################\n\n");

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
        // errno
        handleError(1, 1, 0, 1, "Error in registering exit cleanupper main function.\n");
    }
    printff(NULL, "Exit safe function (cleanup for the main) registered correctly.\n");

    // Enabling the mask.
    retvalue = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
    if (retvalue != 0) {
        // Error
        handleError(0, 1, 0, 1, "Error in setting the pthread signals mask.\n");
    }
    printff(NULL, "Threads signals mask enabled correctly.\n");

    struct sigaction sigusr1; // SIGUSR1 sigaction struct, will be used to handle the game end.
  
    // SIGUSR1 will NOT be blocked.
    // Setting the endGame() handler (does nothing).
    sigusr1.sa_flags = 0;
    sigusr1.sa_handler = endGame;   
    retvalue = sigaction(SIGUSR1, &sigusr1, NULL);
    if (retvalue == -1) {
        // Error
        // errno
        handleError(1, 1, 0, 1, "Error in setting SIGUSR1 signal handler.\n");
    }           
    printff(NULL, "SIGUSR1 signal handler registered correctly.\n");

    // Any newly created threads INHERIT the signal mask with SIGALRM, SIGINT, SIGPIPE signals blocked. 

    // Creating the thread that will handle ALL AND EXCLUSIVELY SIGINT, SIGALRM and SIGPIPE signals
    // by waiting with sigwait() forever in loop.
    retvalue = pthread_create(&sig_thr_id, NULL, signalsThread, NULL);
    if (retvalue != 0) {
        // Error
        handleError(0, 1, 0, 1, "Error in creating the pthread signals handler.\n");
    }

    printff(NULL, "Signals registered and thread handler started succesfully.\n");

    // Check number of args.
    if (argc < 3 || argc > 11 || ((argc > 3) && (argc != 5 && argc != 7 && argc != 9 && argc != 11))) {
        // Error
        handleError(0, 1, 0, 1, USAGE_MSG, argv[0]);
    }

    // Parsing port.
    unsigned long int port = strtoul(argv[2], NULL, 10);
    if (port > 65535LU) {
        // Error
        handleError(0, 1, 0, 1, "Invalid port %lu. It should be less than 65535 and higher than 0.\n", port);
    }
    server_addr.sin_port = htons(port);

    server_addr.sin_family = AF_INET;
    
    // Parsing IP.
    retvalue = parseIP(argv[1], &server_addr);
    if (retvalue != 1) {
        // Error
        handleError(0, 1, 0, 1, "Invalid IP: %s.\n", argv[1]);
    }

    // Checking optionals args.
    char* filemath = NULL;
    char* filedict = NULL;
    for (int i = 3; i < argc; i += 2) {
        // Lowering input.
        toLowerOrUpperString(argv[i], 'L');
        if (strcmp(argv[i], "--matrices") == 0)
            filemath = argv[i + 1];
        else if (strcmp(argv[i], "--duration") == 0) {
            gameduration = (uli) strtoul(argv[i + 1], NULL, 10);
        }else if (strcmp(argv[i], "--seed") == 0){
            seed = (unsigned long) strtoul(argv[i + 1], NULL, 10);
        }else if (strcmp(argv[i], "--dic") == 0){
            filedict = argv[i + 1];
        }else{
            // Error
            handleError(0, 1, 0, 1, USAGE_MSG, argv[0]);
        }
    }
    printff(NULL, "Starting server on IP: %s and port: %lu.\n", argv[1], port);

    // Setting default args and printing infos.
    if (filemath != NULL) {
        matpath = filemath;
        usematrixfile = 1;
        printff(NULL,"Trying to use matrix file at: %s.\n", matpath);
    }else
        printff(NULL, "Using random matrices.\n");
    if (gameduration != 0)
        printff(NULL, "Using INSERTED match time duration %lu minutes.\n", gameduration);
    else {
        gameduration = 3LU;
        printff(NULL, "Using DEFAULT match time duration %lu minutes.\n", gameduration);
    }
    if (seed != 0)
        printff(NULL, "Using INSERTED seed %lu.\n", seed);
    else {
        seed = 42U;
        printff(NULL, "Using DEFAULT seed %lu.\n", seed);
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
                handleError(0, 1, 0, 1, "Error while parsing args with getopt_long().\n");
        }
    }

    if (optind < argc) {
        // Error
        printff(NULL, "Error, unrecognized args: ");
        while (optind < argc) printff(NULL, "%s ", argv[optind++]);
        printff(NULL, "\n");
        printff(NULL, USAGE_MSG, argv[0]);
        handleError(0, 1, 0, 1, "Error, unrecognized args.\n");
    }

    printff(NULL, "The args seems to be ok...\n");

    // Setting rand seed.
    srand(seed);
    printff(NULL, "Initialized random seed %lu.\n", seed);

    // Loading words dictionary in memory.
    if (filedict != NULL)
        printff(NULL, "Trying to use dictionary file at: %s.\n", filedict);
    else{
        filedict = DEFAULT_DICT;
        printff(NULL, "Trying to use default dictionary file at %s.\n", filedict);
    }
    loadDictionary(filedict);
 
    // Initializing game matrix.
    initMatrix();

    // Creating the thread that will handle the disconnections.
    retvalue = pthread_create(&disconnectdetector, NULL, disconnectDetector, NULL);
    if (retvalue != 0) {
        // Error
        handleError(0, 1, 0, 1, "Error in creating the pthread disconnections detector handler.\n");
    }

    printff(NULL, "Disconnections detector pthread handler started succesfully.\n");

    // Creating socket.
    socket_server_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
    if (socket_server_fd == -1) {
        // Error
        handleError(0, 1, 0, 1, "Error in creating the socket.\n");
    }
    printff(NULL, "Socket created.\n");
    
    // Binding socket.
    retvalue = bind(socket_server_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
    if (retvalue == -1){
        // Error
        handleError(0, 1, 0, 1, "Error in binding.\n");
    }
    printff(NULL, "Binding completed.\n");

    // Listening for incoming connections.
    retvalue = listen(socket_server_fd, SOMAXCONN);
    if (retvalue == -1) {
        // Error
        // errno
        handleError(1, 1, 0, 1, "Error in listening.\n");
    }
    printff(NULL, "Listening...\n");

    // Starting the first game.
    startGame();

    // Updating clients (the first time not needed but done for clarity).
    updateClients();

    // Accepting new clients.
    while(1) acceptClient();

    exit(EXIT_SUCCESS);

    return 0;
    
}



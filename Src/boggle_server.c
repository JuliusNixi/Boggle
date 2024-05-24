// Shared server files vars and libs.
#include "server.h"

// Current file vars and libs.
#include <getopt.h>
#define USAGE_MSG "Invalid args. Usage: ./%s nome_server porta_server [--matrici data_filename] [--durata durata_in_minuti] [--seed rnd_seed] [--diz dizionario].\n" // Message to print when the user insert wrong args.
#define DEFAULT_DICT "../Data/dictionary_ita.txt" // Default dict used when --diz is not present.

int main(int argc, char** argv) {

    int retvalue; // To check system calls result (succes or failure).
    unsigned int seed = 0U; // Random seed.

    // Creating a mask, that will block the SIGINT and SIGALRM signals, and enabling
    // it for the current main thread.
    // Important to do it as soon as possible.
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGINT);
    sigaddset(&signal_mask, SIGALRM);
    retvalue = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
    if (retvalue != 0) {
        // Error
        printf("Error in setting the pthread signals mask.\n");
    }

    /* Any newly created threads INHERIT the signal mask with these signals blocked. */

    // Creating the thread that will handle ALL AND EXCLUSIVELY SIGINT and SIGALRM signals by
    // waiting with waitsignal() forever.
    retvalue = pthread_create(&sig_thr_id, NULL, signalsThread, NULL);
    if (retvalue != 0) {
        // Error
        printf("Error in creating the pthread signals handler.\n");
    }

    printf("Signals registered and thread handler started succesfully.\n");

    // Initializing shared vars.
    gameduration = 0LU;
    matpath = NULL;
    usematrixfile = 0;

    // Printing banner.
    printf("\n\n##################\n#     SERVER     #\n##################\n\n");

    // Check number of args.
    if (argc < 3 || argc > 11 || ((argc > 3) && (argc != 5 && argc != 7 && argc != 9 && argc != 11))) {
        // Error
        printf(USAGE_MSG, argv[0]);
    }

    // Registering exit functions.
    retvalue = atexit(clearExit);
    if (retvalue == -1) {
        // Error
        printf("Error during the registration of the safe function exit (cleanup).\n");
    }
    printf("Exit safe exit function (cleanup) registered correctly.\n");

    // Parsing port.
    unsigned int port = atoi(argv[2]);
    if (port > 65535U || (int) port <= 0U) {
        // Error
        printf("Invalid port %u. It should be less than 65535 and higher than 0.\n", port);
    }
    server_addr.sin_port = htons(port);

    server_addr.sin_family = AF_INET;
    
    // Parsing IP.
    retvalue = parseIP(argv[1], &server_addr);
    if (retvalue != 1) {
        // Error
        printf("Invalid IP: %s.\n", argv[1]);
    }

    // Checking optional args.
    char* filemath = NULL;
    char* filedict = NULL;
    for (int i = 3; i < argc; i += 2) {
        toLowerOrUpperString(argv[i], 'l');
        if (strcmp(argv[i], "--matrici") == 0)
            filemath = argv[i + 1];
        else if (strcmp(argv[i], "--durata") == 0) {
            gameduration = atoi(argv[i + 1]);
            if ((int) gameduration <= 0) {
                // Error
                printf("Invalid game duration, cannot be negative %d.\n", (int) gameduration);
            }
        }else if (strcmp(argv[i], "--seed") == 0){
            seed = atoi(argv[i + 1]);
            if ((int) seed <= 0) {
                // Error
                printf("Invalid seed %d. Must be greater than 0.\n", (int) seed);
            }
        }else if (strcmp(argv[i], "--diz") == 0){
            filedict = argv[i + 1];
        }else{
            // Error
            printf(USAGE_MSG, argv[0]);
        }
    }
    printf("Starting server on IP: %s and port: %u.\n", argv[1], port);

    // Setting default args and printing infos.
    if (filemath != NULL) {
        matpath = filemath;
        usematrixfile = 1;
        printf("Trying to use matrix file at: %s.\n", matpath);
    }else
        printf("Using random matrices.\n");
    if (gameduration != 0)
        printf("Using inserted match time duration %lu minutes.\n", gameduration);
    else {
        gameduration = 3LU;
        printf("Using default match time duration %lu minutes.\n", gameduration);
    }
    if (seed != 0)
        printf("Using inserted seed %u.\n", seed);
    else {
        seed = 42U;
        printf("Using default seed %u.\n", seed);
    }

    // Further checks added after the suggestion given by Prof.
    // getopt_long
    // https://man7.org/linux/man-pages/man3/getopt.3.html
    // Starting parsing from 3, because 0 = Program Name, 1 = IP, 2 = Port, 3 = ...
    optind = 3;
    // Suppressing opt library error messages, will be used mine.
    opterr = 0;
    // C will be the character read.
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
                   printf("Error while parsing args with getopt_long().\n");
                   printf(USAGE_MSG, argv[0]);
        }
    }

    if (optind < argc) {
        // Error
        printf("Error, unrecognized args: ");
        while (optind < argc) printf("%s ", argv[optind++]);
        printf("\n");
        printf(USAGE_MSG, argv[0]);
    }

    printf("The args seems to be ok...\n");

    // Setting rand seed.
    srand(seed);
    printf("Initialized random seed %u.\n", seed);

    
    // Loading words dictionary in memory.
    if (filedict != NULL)
        printf("Trying to use dictionary file at: %s.\n", filedict);
    else{
        filedict = DEFAULT_DICT;
        printf("Trying to use default dictionary file at %s.\n", filedict);
    }
    loadDictionary(filedict);

    // Initializing game matrix.
    initMatrix();
    
    // Creating socket.
    socket_server_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
    if (socket_server_fd == -1) {
        // Error
        printf("Error in creating the socket.\n");
    }
    printf("Socket created.\n");
    
    // Binding socket.
    retvalue = bind(socket_server_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
    if (retvalue == -1){
        // Error
        printf("Error in binding.\n");
    }
    printf("Binding completed.\n");

    // Listening for incoming connections.
    retvalue = listen(socket_server_fd, SOMAXCONN);
    if (retvalue == -1) {
        // Error
        printf("Error in listening.\n");
    }
    printf("Listening...\n");

    // Starting the game.
    startGame();

    // Accepting new clients.
    while(1) acceptClient();

    return 0;
    
}



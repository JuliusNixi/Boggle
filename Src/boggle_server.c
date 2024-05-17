// Shared server files vars and libs.
#include "server.h"
extern unsigned int duration;

// Current file vars and libs.
#include <signal.h>

#define USAGE_MSG "Invalid args. Usage: ./%s nome_server porta_server [--matrici data_filename] [--durata durata_in_minuti] [--seed rnd_seed] [--diz dizionario].\n" // Message to print when the user insert wrong args.

#define DEFAULT_DICT "./Data/dictionary_ita.txt" // Default dict used why --diz is not present.

int main(int argc, char** argv) {

    // Initializing shared vars.
    duration = 0U;

    struct sigaction sigint; // SIGINT signal handler.
    struct sigaction sigactiontimer; // Timer that will handle the game time.

    unsigned int seed = 0U; // Random seed.

    // Printing banner.
    printf("\n\n##################\n#     SERVER     #\n##################\n\n");

    // Check number of args.
    if (argc < 3 || argc > 11 || ((argc > 3) && (argc != 5 && argc != 7 && argc != 9 && argc != 11))) {
        // Error
        printf(USAGE_MSG, argv[0]);
    }

    // Registering SIGINT singlal handler.
    int retvalue = 0;
    sigint.sa_handler = sigintHandler;
    retvalue = sigaction(SIGINT, &sigint, NULL);
    if (retvalue == -1) {
        // Error
        printf("Error in setting SIGINT signal handler.\n");
    }
    printf("SIGINT signal handler registered.\n");

    // Parsing port.
    unsigned int port = atoi(argv[2]);
    if (port > 65535U || (int)port <= 0U) {
        // Error
        printf("Invalid port %d. It should be less than 65535 and higher than 0.\n", (int)port);
    }
    server_addr.sin_port = htons(port);

    server_addr.sin_family = AF_INET;
    
    // Parsing IP.
    char* s = argv[1];
    retvalue = parseIP(s, &server_addr);
    if (retvalue != 1) {
        // Error
        printf("Invalid IP %s.\n", argv[1]);
    }

    // Checking optional args.
    char* filemath = NULL;
    char* filedict = NULL;
    for (int i = 3; i < argc; i += 2) {
        s = argv[i];
        toLowerOrUpperString(s, 'l');
        if (strcmp(argv[i], "--matrici") == 0)
            filemath = argv[i + 1];
        else if (strcmp(argv[i], "--durata") == 0) {
            duration = atoi(argv[i + 1]);
            if ((int) duration <= 0) {
                // Error
                printf("Invalid game duration %d.\n", (int) duration);
            }
        }else if (strcmp(argv[i], "--seed")){
            seed = atoi(argv[i + 1]);
            if ((int) seed <= 0) {
                // Error
                printf("Invalid seed %d. Must be greater than 0.\n", (int) seed);
            }
        }else if (strcmp(argv[i], "--diz")){
            filedict = argv[i + 1];
        }else{
            // Error
            printf(USAGE_MSG, argv[0]);
        }
    }
    printf("Starting server on IP: %s and port: %u.\n", argv[1], port);

    // Setting default args and printing infos.
    if (filemath != NULL) 
        printf("Trying to use matrix file at: %s.\n", filemath);
    else
        printf("Using random matrices.\n");
    if (duration != 0)
        printf("Using inserted duration %u minutes.\n", duration);
    else {
        duration = 3U;
        printf("Using default duration %u minutes.\n", duration);
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
    int c; 
    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
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
                    // long_options[option_index].name // Arg name.
                    // optarg // Arg value.
            // All OK, all other controls have been performed previously.
            break;

            case '?':  // Unrecognized/Not allowed character parsed.
            default:
                   // Error
                   printf("Error while parsing args with getopt_long().\n");
                   printf(USAGE_MSG, argv[0]);
                   return 1;
        }
    }

    if (optind < argc) {
         // Error
        printf("Error, unrecognized args: ");
        while (optind < argc) printf("%s ", argv[optind++]);
        printf("\n");
        printf(USAGE_MSG, argv[0]);
        return 1;
    }

    printf("The args seems to be ok...\n");

    // Setting rand seed.
    srand(seed);
    printf("Initialized random seed %u.\n", seed);

    // Registering timer signal handler.
    sigactiontimer.sa_handler = timerHandler;
    retvalue = sigaction(SIGALRM, &sigactiontimer, NULL);
    if (retvalue == -1) {
        // Error
        printf("Error in setting game timer signal handler.\n");
    }
    printf("Game timer signal handler registered.\n");

    // Loading words dictionary in memory.
    if (filedict != NULL){
        printf("Trying to use dictionary file at: %s.\n", filedict);
        loadDictionary(filedict);
    }else{
        printf("Trying to use default dictionary file at %s.\n", DEFAULT_DICT);
        loadDictionary(DEFAULT_DICT);
    } 

    // Initializing game matrix.
    initMatrix();
    
    // Creating socket.
    socket_server_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
    if (socket_server_fd == -1) {
        // Error
        printf("Error in creating socket.\n");
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



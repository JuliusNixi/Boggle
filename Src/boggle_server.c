#include "support_functions.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>

extern struct sockaddr_in server_addr;
extern int socket_server_fd;
extern unsigned int duration;
extern struct sigaction sigactiontimer;
extern unsigned int seed;
extern char DEFAULT_DICT[];

int main(int argc, char** argv) {

    // Printing banner.
    printf("\n\n##################\n#     SERVER     #\n##################\n\n");

    // Check number of args.
    if (argc < 3 || argc > 11 || ((argc > 3) && (argc != 5 && argc != 7 && argc != 9 && argc != 11))) {
        // Error
        printf("Invalid args. Usage: ./%s nome_server porta_server [--matrici data_filename] [--durata durata_in_minuti] [--seed rnd_seed] [--diz dizionario].\n", argv[0]);
    }

    // Parsing port.
    unsigned int port = atoi(argv[2]);
    if (port > 65535 || port <= 0) {
        // Error
        printf("Invalid port %u. It should be less than 65535 and higher than 0.\n", port);
    }
    server_addr.sin_port = htons(port);

    server_addr.sin_family = AF_INET;
    
    // Parsing IP.
    int retvalue = 0;
    if (strcmp(argv[1], "localhost") == 0)
        retvalue = inet_aton("127.0.0.1", &(server_addr.sin_addr));
    else
        retvalue = inet_aton(argv[1], &(server_addr.sin_addr));
    if (retvalue != 1) {
        // Error
        printf("Invalid IP %s.\n", argv[1]);
    }

    // Checking optional args.
    char* filemath = NULL;
    char* filedict = NULL;
    for (int i = 3; i < argc; i += 2)
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
                printf("Invalid seed %d. Must be greater than 0\n", (int) seed);
            }
        }else if (strcmp(argv[i], "--diz")){
            filedict = argv[i + 1];
        }else{
            // Error
            printf("Invalid args. Usage: ./%s nome_server porta_server [--matrici data_filename] [--durata durata_in_minuti] [--seed rnd_seed] [--diz dizionario].\n", argv[0]);
        }

    printf("Starting server on IP: %s and port: %d.\n", argv[1], port);

    // Setting default args and printing infos.
    if (filemath != NULL) 
        printf("Trying to use matrix file at: %s.\n", filemath);
    else
        printf("Using random matrices.\n");
    if ((int) duration != 0)
        printf("Using inserted duration %u minutes.\n", duration);
    else {
        duration = 3U;
        printf("Using default duration %u minutes.\n", duration);
    }
    if ((int) seed != 0)
        printf("Using inserted seed %u.\n", seed);
    else {
        seed = 42U;
        printf("Using default seed %u.\n", seed);
    }
    if (filedict != NULL) 
        printf("Trying to use dictionary file at: %s.\n", filedict);
    else
        printf("No dictionary has been set.\n");

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
                   return 0;
                   break;
        }
    }

    if (optind < argc) {
         // Error
         printf("Error, unrecognized args: ");
        while (optind < argc) printf("%s ", argv[optind++]);
        printf("\n");
        return 0;
    }

    printf("The args seems to be ok...\n");

    // Setting rand seed.
    srand(seed);
    printf("Initialized random seed %u.\n", seed);

    // Registering timer handler.
    sigactiontimer.sa_handler = timerHandler;
    retvalue = sigaction(SIGALRM, &sigactiontimer, NULL);
    if (retvalue == -1) {
        // Error
        printf("Error in setting game timer signal handler.\n");
    }
    printf("Game timer signal handler registered.\n");

    // Loading words dictionary in memory.
    if (filedict != NULL)
        loadDictionary(filedict);
    else{
        if ((DEFAULT_DICT != NULL)) {
            printf("Using default dictionary file.\n");
            loadDictionary(DEFAULT_DICT);
        }else{
            printf("WARNING: Starting the game without a dictionary file!\nThe submitted words will be searched only in the current game matrix!\n");
        }
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



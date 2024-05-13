#include "support_functions.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

extern struct sockaddr_in server_addr;
extern int socket_server_fd;
extern unsigned int duration;
extern struct sigaction sigactiontimer;

int main(int argc, char** argv) {

    // Printing banner.
    printf("\n\n##################\n#     SERVER     #\n##################\n\n");

    // Check number of args.
    if (argc < 3 || argc > 7 || ((argc > 3) && (argc != 5 && argc != 7))) {
        // Error
        printf("Invalid args. Usage: ./%s IP PORT [--matrici FILEPATH] [--durata MINUTES].\n", argv[0]);
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
    char* filepath = NULL;
    int customduration = 0;
    for (int i = 3; i < argc; i += 2)
        if (strcmp(argv[i], "--matrici") == 0)
            filepath = argv[i + 1];
        else if (strcmp(argv[i], "--durata") == 0) {
            duration = atoi(argv[i + 1]);
            customduration = 1;
            if (duration <= 0) {
                // Error
                printf("Invalid game duration %d.\n", duration);
            }
        }else{
            // Error
            printf("Invalid args. Usage: ./%s IP PORT [--matrici FILEPATH] [--durata MINUTES].\n", argv[0]);
        }

    printf("Starting server on IP: %s and port: %d.\n", argv[1], port);

    // Printing infos.
    if (filepath != NULL) 
        printf("Trying to use matrix file at: %s.\n", filepath);
    if (customduration)
        printf("Using inserted duration %d minutes.\n", duration);
    else 
        printf("Using default duration %d minutes.\n", duration);

    printf("The args seems to be correct...\n");

    // Registering timer handler.
    sigactiontimer.sa_handler = timerHandler;
    retvalue = sigaction(SIGALRM, &sigactiontimer, NULL);
    if (retvalue == -1) {
        // Error
        printf("Error in setting game timer signal handler.\n");
    }
    printf("Game timer signal handler registered.\n");

    // Loading words dictionary in memory.
    loadDictionary();

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



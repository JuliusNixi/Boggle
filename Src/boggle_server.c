#include "support_functions.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

extern struct sockaddr_in server_addr;
extern int socket_server_fd;
extern int duration;
extern struct sigaction sigactiontimer;

int main(int argc, char** argv) {

    // Printing banner.
    printf("\n\n\n##################\n#     SERVER     #\n##################\n\n\n");

    // Check number of args.
    if (argc < 3 || argc > 7) {
        // Error
    }

    // Parsing port.
    int port = atoi(argv[2]);
    if (port < 65535 || port <= 0) {
        // Error
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
        printf("Invalid ip %d.\n", retvalue);
    }

    if ((argc >= 3) && (argc != 5 && argc != 7)) {
        // Error
    }

    // Checking optional args.
    char* filepath = NULL;
    for (int i = 3; i < argc; i += 2)
        if (strcmp(argv[i], "--matrici") == 0) {
            filepath = argv[i + 1];
        }
        else if (strcmp(argv[i], "--durata") == 0) {
            duration = atoi(argv[i + 1]);
            if (duration <= 0) {
                // Error
            }
        }else{
            // Error
        }

    printf("Starting server on IP: %s and port: %d.\n", argv[1], port);

    // Printing infos.
    if (filepath != NULL) 
        printf("Using matrix file at: %s.\n", filepath);
    if (duration != 0)
        printf("Using duration %d seconds.\n", duration);
    
    // Creating socket.
    socket_server_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
    if (socket_server_fd == -1) {
        // Error
    }
    printf("Socket created.\n");
    
    // Binding socket.
    retvalue = bind(socket_server_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
    if (retvalue == -1){
        // Error
    }
    printf("Binding completed.\n");

    // Registering timer handler.
    sigactiontimer.sa_handler = timerHandler;
    retvalue = sigaction(SIGALRM, &sigactiontimer, NULL);
    if (retvalue == -1) {
        // Error
    }
    printf("Timer handler registered.\n");

    // Listening for incoming connections.
    retvalue = listen(socket_server_fd, SOMAXCONN);
    if (retvalue == -1) {
        // Error
    }
    printf("Listening...\n");

    // Starting the game.
    startGame();

    // Accepting new clients.
    while(1) acceptClient();

    return 0;
    
}



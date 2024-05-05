#include "support_functions.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern struct sockaddr_in socket_server;
extern int socket_server_fd;

int main(int argc, char** argv) {

    // Check number of args.
    if (argc < 3 || argc > 7) {
        // Error
    }

    // Parsing port.
    int port = atoi(argv[2]);
    if (port < 65535 || port <= 0) {
        // Error
    }
    socket_server.sin_port = htons(port);

    socket_server.sin_family = AF_INET;
    
    // Parsing ip.
    int retvalue = 0;
    if (strcmp(argv[1], "localhost") == 0)
        retvalue = inet_aton("127.0.0.1", &(socket_server.sin_addr));
    else
        retvalue = inet_aton(argv[1], &(socket_server.sin_addr));
    if (retvalue != 1) {
        // Error
        printf("Invalid ip %d.\n", retvalue);
    }

    if ((argc >= 3) && (argc != 5 && argc != 7)) {
        // Error
    }

    char* filepath = NULL;
    int duration = 0;
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
    
    printf("Socket created...\n");
    socket_server_fd = socket(socket_server.sin_family, SOCK_STREAM, 0);
    if (socket_server_fd == -1) {
        // Error
    }
    
    printf("Binding completed...\n");
    retvalue = bind(socket_server_fd, (const struct sockaddr*) &socket_server, (socklen_t) sizeof(socket_server));
    if (retvalue == -1){
        // Error
    }

    printf("Listening...\n");
    retvalue = listen(socket_server_fd, SOMAXCONN);
    if (retvalue == -1) {
        // Error
    }

/*
    int fd_c;
    sysc(fd_c, accept(
        fd_skt, // int fd_skt filedescriptor socket 
        (struct sockaddr *) &ca, // (const struct sockaddr*) !CASTING! ca indirizzo o NULL 
        &client_addr_len // socklen_t sa_len (cioè sizeof(ca)) lunghezza indirizzo 
    ), "Errore nella accept");
    // (fdc) success (-1) error, sets errno 
    // Se sa_len != NULL al ritorno contiene l'indirizzo del socket che ha accettato la connessione e sa_len la
    // lunghezza della struttura. fdc è il descriptor del nuovo socket che sarù usato per la comunicazione 
*/
    
    return 0;
    
}

/*
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include "macro.h"
#include <stdlib.h>
#include <stdio.h>
*/


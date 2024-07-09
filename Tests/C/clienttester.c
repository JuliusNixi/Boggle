#include "../../Src/Common/common.h"

// Remember to compile with also "../../Src/Common/common.c".

// This file is a very basic client tester.
// It's a simplified server, which is intended to test the client abstracting
// from the complex operations that the real project server should have.
// It simply accepts one client, wait a random time in seconds, and write a fixed message.

int main(void) {

    // Setting port.
    uli port = 8080LU;
    server_addr.sin_port = htons(port);

    server_addr.sin_family = AF_INET;
    
    // Setting IP.
    inet_aton("127.0.0.1", &server_addr.sin_addr);

    int socket_server_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
    
    bind(socket_server_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));;

    listen(socket_server_fd, SOMAXCONN);

    // Client's acceptance.
    struct sockaddr_in client_addr;
    socklen_t client_address_len;
    client_address_len = (socklen_t) sizeof(client_addr);
    int socket_client_fd = accept(socket_server_fd, (struct sockaddr*) (&(client_addr)), &(client_address_len));

    printf("Client accepted!\n");

    char teststring[] = "TEST STRING!\n";

    srand(RAND_SEED);

    while (1){
        // Sleeping random seconds.
        int randint = rand() % 5;
        sleep(randint);
        // 1, if returned, means OK.
        char r = sendMessage(socket_client_fd, MSG_OK, teststring);
        printf("Result: %d.\n", (int) r);
    }
    
    return 0;

}


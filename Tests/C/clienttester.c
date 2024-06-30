#include "../../Src/Common/common.h"

// Compile with "../../Src/Common/common.c" file.

int main(void) {

    uli port = 8080LU;

    server_addr.sin_port = htons(port);

    server_addr.sin_family = AF_INET;
    
    inet_aton("127.0.0.1", &server_addr.sin_addr);

    int socket_server_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
    
    bind(socket_server_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));;

    listen(socket_server_fd, SOMAXCONN);

    struct sockaddr_in client_addr;
    socklen_t client_address_len;
    client_address_len = (socklen_t) sizeof(client_addr)
    int socket_client_fd = accept(socket_server_fd, (struct sockaddr*) (&(client_addr)), &(client_address_len));

    printf("Client accepted!\n");

    char teststring[] = "TEST STRING!\n";

    srand(42);

    while (1){
        // Sleeping random seconds.
        int randint = rand() % 5;
        sleep(randint);
        // 1 means OK.
        char r = sendMessage(socket_client_fd, MSG_OK, teststring);
        printf("Result: %d.\n", (int) r);
    }
    
    return 0;

}


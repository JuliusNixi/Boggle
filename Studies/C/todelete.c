#include "../../Src/Common/common.h"

#include <sys/ioctl.h>

#define STR_TO_SEND "Hello World!\n"

// Remember to compile with also "../../Src/Common/common.c".

// Server.
int socket_server_fd;
int socket_client_fd;
struct sockaddr_in client_addr; // Client address.
socklen_t client_address_len; // Client address length.

// Client.
int client_fd;

int main(void) {

    // Parsing port.
    uli port = 8080LU;

    server_addr.sin_port = htons(port);

    // Socket type.
    server_addr.sin_family = AF_INET;
    
    // Parsing IP.
    char ip[] = "localhost";
    int retvalue = parseIP(ip, &server_addr);
    if (retvalue != 1) {
        // Error
    }

    pid_t p;
    p = fork();
    if (p == -1){
        // Error
    }

    if (p == 0) {
        // Son.

        // Creating socket.
        client_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
        if (client_fd == -1) {
            // Error
        }
        fprintf(stdout, "Socket created.\n");

        retvalue = connect(client_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        if (retvalue == -1){
            retvalue = close(client_fd);
            if (retvalue == -1) {
                // Error
            }
            fprintf(stdout, "Socket closed.\nError in connecting.\n");
            exit(1);
        }
        fprintf(stdout, "Connected succesfully!\n");

        sendMessage(client_fd, MSG_OK, STR_TO_SEND);

        while(1) sleep(1);

    }else{
        // Father.

        // Creating socket.
        // READ man socket, there are useful infos.
        socket_server_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
        if (socket_server_fd == -1) {
            // Error
        }
        fprintf(stdout, "Server socket created.\n");
        
        // Binding socket.
        retvalue = bind(socket_server_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        if (retvalue == -1){
            // Error
        }
        fprintf(stdout, "Binding completed.\n");

        // Listening for incoming connections.
        retvalue = listen(socket_server_fd, SOMAXCONN);
        if (retvalue == -1) {
            // Error
        }
        fprintf(stdout, "Listening...\n");

        socket_client_fd = accept(socket_server_fd, (struct sockaddr*) (&(client_addr)), &(client_address_len));
        if (socket_client_fd == -1) {
            // Error
        }
        fprintf(stdout, "Accepted a new client.\n");

        sleep(3);

        int count;
        retvalue = ioctl(socket_client_fd, FIONREAD, &count);
        if (retvalue == -1) {
            // Error
        }
        uli sent = sizeof(MSG_OK) + sizeof(unsigned) + ((uli) strlen(STR_TO_SEND) + 1);
        fprintf(stdout, "The client has sent %lu bytes.\n", sent);
        fprintf(stdout, "In the socket there are %d bytes.\n", count);
        fprintf(stdout, "Are equals? %d.\n", count == sent);
        char returncode;
        struct Message* m = receiveMessage(socket_client_fd, &returncode);
        fprintf(stdout, "The message received is: %s", m->data);
        retvalue = ioctl(socket_client_fd, FIONREAD, &count);
        if (retvalue == -1) {
            // Error
        }
        fprintf(stdout, "In the socket there are %d bytes.\n", count);

        exit(0);

    }

    return 0;

}






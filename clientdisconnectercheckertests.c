#include "./Src/Common/common.h"
// Needed for waitpid() on Linux.
#include <sys/types.h>
#include <sys/wait.h>

// Remember to compile with also "../../Src/Common/common.c".

// Server.
int socket_server_fd;
int socket_client_fd;
struct sockaddr_in client_addr; // Client address.
socklen_t client_address_len; // Client address length.

// Client.
int client_fd;

void* clientDisconnecterChecker(void* args) {

    fprintf(stdout, "I'm the clientDisconnecter() thread!\n");
    sleep(3);
    fprintf(stdout, "Sleep finished.\n");
    char resultcode = sendMessage(socket_client_fd, MSG_OK, "Test!\n");
    fprintf(stdout, "sendMessage() resultcode: %d.\n", (int) resultcode);

    pthread_exit(NULL);
    return NULL;

}

int main(void) {

    sigemptyset(&signalmask);
    sigaddset(&signalmask, SIGPIPE);

    // Enabling the signals' mask.
    int retvalue = pthread_sigmask(SIG_BLOCK, &signalmask, NULL);
    if (retvalue != 0) {
        // Error
    }
    fprintf(stdout, "Threads signals mask enabled correctly.\n");

    // Parsing port.
    uli port = 8080LU;

    server_addr.sin_port = htons(port);

    // Socket type.
    server_addr.sin_family = AF_INET;
    
    // Parsing IP.
    char ip[] = "localhost";
    retvalue = parseIP(ip, &server_addr);
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

        char type = 'R';
        retvalue = write(client_fd, &type, 1);
        if (retvalue == -1) {
            // Error
        }else if (retvalue == 1) {
            fprintf(stdout, "Message's type sent.\n");
        }else{
            // Error
        }
        fprintf(stdout, "Son exiting.\n");

        exit(1);

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

        pthread_t t;
        retvalue = pthread_create(&t, NULL, clientDisconnecterChecker, NULL);
        if (retvalue != 0) {
            // Error
        }
        fprintf(stdout, "Thread created!\n");

        char resultcode;
        struct Message* message;
        message = receiveMessage(socket_client_fd, &resultcode);
        fprintf(stdout, "Ok, receiveMessage() resultcode: %d.\n", (int) resultcode);

        int status;
        waitpid(p, &status, 0);
        fprintf(stdout, "Son process waited. Father exiting...\n");

        exit(0);

    }


    return 0;

}






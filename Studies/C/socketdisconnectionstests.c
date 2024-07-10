
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
// Needed for waitpid() on Linux.
#include <sys/types.h>
#include <sys/wait.h>

// This file contains some tests on the socket disconnection and related errors.
// https://stackoverflow.com/questions/33053507/econnreset-in-send-linux-c

void sigPIPEHandler(int signum) {

    pid_t p = getpid();
    // Not safe in a handler, but for these tests I will take the risk...
    printf("SIGPIPE HANDLER PID: %d.\n", p);

}

void clientForcedDisconnection(void) {

    unsigned long port = 8080LU;
    struct sockaddr_in server_addr;
    server_addr.sin_port = htons(port);
    server_addr.sin_family = AF_INET;
    char ip[] = "127.0.0.1";
    inet_aton(ip, &server_addr.sin_addr);

    pid_t p = fork();
    if (p == 0) {
        // Son.
        sleep(1);
        int client_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
        printf("Socket created client.\n");
        connect(client_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        printf("Connected succesfully!\n");
        char x = 'X';
        write(client_fd, &x, 1);
        printf("Message sent by the client: %c!\n", x);
        printf("Client exiting forcibly...\n");
        exit(EXIT_SUCCESS);
    }else{
        // Father.
        int socket_server_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
        printf("Socket created server.\n");
        bind(socket_server_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        printf("Binding complete.\n");
        listen(socket_server_fd, SOMAXCONN);
        printf("Listening...\n");
        socklen_t l = (socklen_t) sizeof(server_addr);
        int socket_client_fd = accept(socket_server_fd, (struct sockaddr*) (&(server_addr)), &l);
        printf("Client accepted by the server.\n");
        while (1) {
            // Reading a char.
            char x = '-';
            int retvalue = read(socket_client_fd, &x, 1);
            printf("Readed by the server (code: %d): %c.\n", retvalue, x);
            if (retvalue <= 0) {
                printf("Errno code: %d.\n", (int) errno);
                perror("Errno message");
                fflush(stdout);
                break;
            } 
            sleep(1);
        }
        printf("Server exiting, BEFORE CLOSING FD...\n");
        close(socket_client_fd);
        close(socket_server_fd);
        printf("Waiting for the son process...\n");
        fflush(stdout);
        waitpid(p, NULL, 0);
    }

    return;

}

void clientCorrectDisconnection(void) {

    unsigned long port = 8081LU;
    struct sockaddr_in server_addr;
    server_addr.sin_port = htons(port);
    server_addr.sin_family = AF_INET;
    char ip[] = "127.0.0.1";
    inet_aton(ip, &server_addr.sin_addr);

    pid_t p = fork();
    if (p == 0) {
        // Son.
        sleep(1);
        int client_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
        printf("Socket created client.\n");
        connect(client_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        printf("Connected succesfully!\n");
        char x = 'X';
        write(client_fd, &x, 1);
        printf("Message sent by the client: %c!\n", x);
        printf("Client exiting, BEFORE CLOSING FD...\n");
        close(client_fd);
        exit(EXIT_SUCCESS);
    }else{
        // Father.
        int socket_server_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
        printf("Socket created server.\n");
        bind(socket_server_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        printf("Binding complete.\n");
        listen(socket_server_fd, SOMAXCONN);
        printf("Listening...\n");
        socklen_t l = (socklen_t) sizeof(server_addr);
        int socket_client_fd = accept(socket_server_fd, (struct sockaddr*) (&(server_addr)), &l);
        printf("Client accepted by the server.\n");
        while (1) {
            // Reading a char.
            char x = '-';
            int retvalue = read(socket_client_fd, &x, 1);
            printf("Readed by the server (code: %d): %c.\n", retvalue, x);
            if (retvalue <= 0) {
                printf("Errno code: %d.\n", (int) errno);
                perror("Errno message");
                break;
            } 
            fflush(stdout);
            sleep(1);
        }
        printf("Server exiting, BEFORE CLOSING FD...\n");
        close(socket_client_fd);
        close(socket_server_fd);
        printf("Waiting for the son process...\n");
        fflush(stdout);
        waitpid(p, NULL, 0);
    }

    return;

}


void serverForcedDisconnection(void) {

    unsigned long port = 8082LU;
    struct sockaddr_in server_addr;
    server_addr.sin_port = htons(port);
    server_addr.sin_family = AF_INET;
    char ip[] = "127.0.0.1";
    inet_aton(ip, &server_addr.sin_addr);

    pid_t p = fork();
    if (p == 0) {
        // Son.
        sleep(1);
        int client_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
        printf("Socket created client.\n");
        int retvalue = connect(client_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        printf("Connected succesfully!\n");
        char x = 'X';
        write(client_fd, &x, 1);
        printf("Message sent by the client: %c!\n", x);

        // Now the client will read something from the server!
        printf("Client reading!\n");
        x = '-';
        retvalue = read(client_fd, &x, 1);
        printf("Readed by the client (code: %d): %c.\n", retvalue, x);
        if (retvalue <= 0) {
            printf("Read client error...\n");
            printf("Errno code: %d.\n", (int) errno);
            perror("Errno message");
        } 
        printf("Client exiting, BEFORE CLOSING FD...\n");
        close(client_fd);
        exit(EXIT_SUCCESS);
    }else{
        // Father.
        // Now two processes for the server are needed because, one will die cause the error we generate.
        pid_t p2;
        p2 = fork();
        if (p2 == 0) {
            // Son.
            int socket_server_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
            printf("Socket created server.\n");
            bind(socket_server_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
            printf("Binding complete.\n");
            listen(socket_server_fd, SOMAXCONN);
            printf("Listening...\n");
            socklen_t l = (socklen_t) sizeof(server_addr);
            int socket_client_fd = accept(socket_server_fd, (struct sockaddr*) (&(server_addr)), &l);
            printf("Client accepted by the server.\n");
            while (1) {
                // Reading a char.
                char x;
                int retvalue = read(socket_client_fd, &x, 1);
                printf("Readed by the server (code: %d): %c.\n", retvalue, x);
                break;
            }
            printf("Server exiting forcibly...\n");
            exit(EXIT_SUCCESS);
        }else{
            printf("Waiting sons processes.\n");
            fflush(stdout);
            waitpid(p2, NULL, 0);
            waitpid(p, NULL, 0);
            printf("Waited OK.\n");
        }
    }

    return;

}


void serverCorrectDisconnection(void) {

    unsigned long port = 8083LU;
    struct sockaddr_in server_addr;
    server_addr.sin_port = htons(port);
    server_addr.sin_family = AF_INET;
    char ip[] = "127.0.0.1";
    inet_aton(ip, &server_addr.sin_addr);

    pid_t p = fork();
    if (p == 0) {
        // Son.
        sleep(1);
        int client_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
        printf("Socket created client.\n");
        connect(client_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        printf("Connected succesfully!\n");
        char x = 'X';
        write(client_fd, &x, 1);
        printf("Message sent by the client: %c!\n", x);

        // Now the client will read something from the server!
        printf("Client reading!\n");
        x = '-';
        int retvalue = read(client_fd, &x, 1);
        printf("Readed by the client (code: %d): %c.\n", retvalue, x);
        if (retvalue <= 0) {
            printf("Read client error...\n");
            printf("Errno code: %d.\n", (int) errno);
            perror("Errno message");
        } 
        printf("Client exiting, BEFORE CLOSING FD...\n");
        close(client_fd);
        exit(EXIT_SUCCESS);
    }else{
        // Father.
        // Now two processes for the server are needed because, one will die cause the error we generate.
        pid_t p2;
        p2 = fork();
        if (p2 == 0) {
            // Son.
            int socket_server_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
            printf("Socket created server.\n");
            bind(socket_server_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
            printf("Binding complete.\n");
            listen(socket_server_fd, SOMAXCONN);
            printf("Listening...\n");
            socklen_t l = (socklen_t) sizeof(server_addr);
            int socket_client_fd = accept(socket_server_fd, (struct sockaddr*) (&(server_addr)), &l);
            printf("Client accepted by the server.\n");
            while (1) {
                // Reading a char.
                char x;
                int retvalue = read(socket_client_fd, &x, 1);
                printf("Readed by the server (code: %d): %c.\n", retvalue, x);
                break;
            }
            printf("Server exiting, BEFORE CLOSING FD...\n");
            close(socket_client_fd);
            close(socket_server_fd);
            exit(EXIT_SUCCESS);
        }else{
            printf("Waiting sons processes.\n");
            fflush(stdout);
            waitpid(p2, NULL, 0);
            waitpid(p, NULL, 0);
            printf("Waited OK.\n");
        }
    }
}

void clear(void) {
    
    printf("\n\n----------------------------------------------------\n\n");
    return;

}

void unhandledData(void) {

    unsigned long port = 8084LU;
    struct sockaddr_in server_addr;
    server_addr.sin_port = htons(port);
    server_addr.sin_family = AF_INET;
    char ip[] = "127.0.0.1";
    inet_aton(ip, &server_addr.sin_addr);

    pid_t p = fork();
    if (p == 0) {
        // Son.
        sleep(1);
        int client_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
        printf("Socket created client.\n");
        connect(client_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        printf("Connected succesfully!\n");
        char x = 'X';
        write(client_fd, &x, 1);
        printf("Message sent by the client: %c!\n", x);
        printf("Client exiting...\n");
        exit(EXIT_SUCCESS);
    }else{
        // Father.
        int socket_server_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
        printf("Socket created server.\n");
        bind(socket_server_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        printf("Binding complete.\n");
        listen(socket_server_fd, SOMAXCONN);
        printf("Listening...\n");
        socklen_t l = (socklen_t) sizeof(server_addr);
        int socket_client_fd = accept(socket_server_fd, (struct sockaddr*) (&(server_addr)), &l);
        printf("Client accepted by the server.\n");
        while (1) {
            // Reading a char.
            char x = '-';
            int retvalue = read(socket_client_fd, &x, 1);
            printf("Readed by the server (code: %d): %c.\n", retvalue, x);
            if (retvalue <= 0) {
                printf("Errno code: %d.\n", (int) errno);
                perror("Errno message");
                fflush(stdout);
                break;
            }
            // Reply back, this reply will go to client buffer,
            // but the client will close the socket without reading it.
            char n = 'N';
            write(socket_client_fd, &n, 1);
            sleep(1);
        }
        printf("Server exiting, BEFORE CLOSING FD...\n");
        close(socket_client_fd);
        close(socket_server_fd);
        fflush(stdout);
        waitpid(p, NULL, 0);
        printf("Waited OK.\n");
    }

    return;

}

void noData(void) {

    unsigned long port = 8085LU;
    struct sockaddr_in server_addr;
    server_addr.sin_port = htons(port);
    server_addr.sin_family = AF_INET;
    char ip[] = "127.0.0.1";
    inet_aton(ip, &server_addr.sin_addr);

    pid_t p = fork();
    if (p == 0) {

        // SIGPIPE signal handler.
        struct sigaction sigp; 
        sigp.sa_flags = 0;
        sigp.sa_handler = sigPIPEHandler;   
        sigaction(SIGPIPE, &sigp, NULL);

        // Son.
        sleep(1);
        int client_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
        printf("Socket created client.\n");
        connect(client_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        printf("Connected succesfully!\n");
        char x = 'X';
        write(client_fd, &x, 1);
        printf("Message sent by the client: %c!\n", x);

        printf("Waiting 5 seconds, to permit the server to quit.\n");
        sleep(5);
        int retvalue = write(client_fd, &x, 1);
        printf("This should be OK... %d\n", retvalue);
        printf("Waiting others 5 seconds NECESSARY otherwise SIGPIPE will NOT be throw.\n");
        sleep(5);
        printf("This should trigger EPIPE. The handler is registered for all the son processes (like this one), so it will be executed and the process won't die.\n");
        retvalue = write(client_fd, &x, 1);
        printf("****************************\n");
        perror("PIPE PERROR MESSAGE");
        printf("ERRNO VALUE: %d. CODE: %d.\n", (int) errno, retvalue);
        printf("****************************\n");
        printf("Client exiting...\n");
        exit(EXIT_SUCCESS);
    }else{
        // Father.
        int socket_server_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
        printf("Socket created server.\n");
        bind(socket_server_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        printf("Binding complete.\n");
        listen(socket_server_fd, SOMAXCONN);
        printf("Listening...\n");
        socklen_t l = (socklen_t) sizeof(server_addr);
        int socket_client_fd = accept(socket_server_fd, (struct sockaddr*) (&(server_addr)), &l);
        printf("Client accepted by the server.\n");
        while (1) {
            // Reading a char.
            char x = '-';
            int retvalue = read(socket_client_fd, &x, 1);
            printf("Readed by the server (code: %d): %c.\n", retvalue, x);
            if (retvalue <= 0) {
                printf("Errno code: %d.\n", (int) errno);
                perror("Errno message ");
                fflush(stdout);
                break;
            } 
            break;
            sleep(1);
        }
        printf("Server exiting, BEFORE CLOSING FD...\n");
        close(socket_client_fd);
        close(socket_server_fd);
        fflush(stdout);
        waitpid(p, NULL, 0);
        printf("Waited OK.\n");
    }

    return;

}


int main(void) {

    // errno == ETIMEDOUT == 60 and retvalue == 0
    clientForcedDisconnection();
    clear();    
    clientCorrectDisconnection();
    clear();
    serverForcedDisconnection();
    clear();
    serverCorrectDisconnection();
    clear();
    // errno == ECONNRESET == 54 and retvalue == -1
    unhandledData();
    clear();
    // errno == EPIPE == 32 and retvalue == -1
    noData();

    return 0;
    
}




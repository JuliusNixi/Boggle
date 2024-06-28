
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

// This file contains some tests on the socket disconnection and related errors.
// https://stackoverflow.com/questions/33053507/econnreset-in-send-linux-c


void clientForcedDisconnection(void) {

    int retvalue = 0;
    unsigned long int port = 8080LU;
    struct sockaddr_in server_addr;
    server_addr.sin_port = htons(port);
    server_addr.sin_family = AF_INET;
    char ip[] = "127.0.0.1";
    inet_aton(ip, &server_addr.sin_addr);

    pid_t p = fork();
    if (p == 0) {
        // Son.
        sleep(3);
        int client_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
        printf("Socket created client.\n");
        sleep(2);
        retvalue = connect(client_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        printf("Connected succesfully!\n");
        char x = 'X';
        write(client_fd, &x, 1);
        printf("Message sent by the client: %c!\n", x);
        sleep(5);
        printf("Client exiting...\n");
        exit(EXIT_SUCCESS);
    }else{
        // Father.
        int socket_server_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
        printf("Socket created server.\n");
        retvalue = bind(socket_server_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        printf("Binding complete.\n");
        retvalue = listen(socket_server_fd, SOMAXCONN);
        printf("Listening...\n");
        sleep(4);
        socklen_t l = (socklen_t) sizeof(server_addr);
        int socket_client_fd = accept(socket_server_fd, (struct sockaddr*) (&(server_addr)), &l);
        printf("Client accepted by the server.\n");
        while (1) {
            // Reading a char.
            char x = '-';
            retvalue = read(socket_client_fd, &x, 1);
            printf("Readed by the server (code: %d): %c.\n", retvalue, x);
            if (retvalue <= 0) {
                printf("Errno code: %d.\n", (int)errno);
                perror("Errno: ");
                fflush(stdout);
                break;
            } 
            fflush(stdout);
            sleep(1);
        }
        printf("Server exiting...\n");
        close(socket_client_fd);
        close(socket_server_fd);
        fflush(stdout);
        waitpid(p, NULL, 0);
    }

    return;

}

void clientCorrectDisconnection(void) {

    int retvalue = 0;
    unsigned long int port = 8081LU;
    struct sockaddr_in server_addr;
    server_addr.sin_port = htons(port);
    server_addr.sin_family = AF_INET;
    char ip[] = "127.0.0.1";
    inet_aton(ip, &server_addr.sin_addr);

    pid_t p = fork();
    if (p == 0) {
        // Son.
        sleep(3);
        int client_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
        printf("Socket created client.\n");
        sleep(2);
        retvalue = connect(client_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        printf("Connected succesfully!\n");
        char x = 'X';
        write(client_fd, &x, 1);
        printf("Message sent by the client: %c!\n", x);
        sleep(5);
        printf("Client exiting closing fd...\n");
        close(client_fd);
        exit(EXIT_SUCCESS);
    }else{
        // Father.
        int socket_server_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
        printf("Socket created server.\n");
        retvalue = bind(socket_server_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        printf("Binding complete.\n");
        retvalue = listen(socket_server_fd, SOMAXCONN);
        printf("Listening...\n");
        sleep(4);
        socklen_t l = (socklen_t) sizeof(server_addr);
        int socket_client_fd = accept(socket_server_fd, (struct sockaddr*) (&(server_addr)), &l);
        printf("Client accepted by the server.\n");
        while (1) {
            // Reading a char.
            char x = '-';
            retvalue = read(socket_client_fd, &x, 1);
            printf("Readed by the server (code: %d): %c.\n", retvalue, x);
            if (retvalue <= 0) {
                printf("Errno code: %d.\n", (int)errno);
                perror("Errno: ");
                break;
            } 
            fflush(stdout);
            sleep(1);
        }
        printf("Server exiting...\n");
        close(socket_client_fd);
        close(socket_server_fd);
        waitpid(p, NULL, 0);
    }

    return;

}


void serverForcedDisconnection(void) {

    int retvalue = 0;
    unsigned long int port = 8082LU;
    struct sockaddr_in server_addr;
    server_addr.sin_port = htons(port);
    server_addr.sin_family = AF_INET;
    char ip[] = "127.0.0.1";
    inet_aton(ip, &server_addr.sin_addr);

    pid_t p = fork();
    if (p == 0) {
        // Son.
        sleep(3);
        int client_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
        printf("Socket created client.\n");
        sleep(2);
        retvalue = connect(client_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        printf("Connected succesfully!\n");
        char x = 'X';
        write(client_fd, &x, 1);
        printf("Message sent by the client: %c!\n", x);
        sleep(5);

        x = '-';
        retvalue = read(client_fd, &x, 1);
        printf("Readed by the client (code: %d): %c.\n", retvalue, x);
        if (retvalue <= 0) {
            printf("Read client error...\n");
            printf("Errno code: %d.\n", (int)errno);
            perror("Errno: ");
        } 
        printf("Client exiting closing fd...\n");
        close(client_fd);
        exit(EXIT_SUCCESS);
    }else{
        // Father.
        pid_t p2;
        p2 = fork();
        if (p2 == 0) {
            // Son.
            int socket_server_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
            printf("Socket created server.\n");
            retvalue = bind(socket_server_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
            printf("Binding complete.\n");
            retvalue = listen(socket_server_fd, SOMAXCONN);
            printf("Listening...\n");
            sleep(4);
            socklen_t l = (socklen_t) sizeof(server_addr);
            int socket_client_fd = accept(socket_server_fd, (struct sockaddr*) (&(server_addr)), &l);
            printf("Client accepted by the server.\n");
            while (1) {
                // Reading a char.
                char x;
                retvalue = read(socket_client_fd, &x, 1);
                printf("Readed by the server (code: %d): %c.\n", retvalue, x);
                break;
            }
            printf("Server exiting...\n");
            exit(EXIT_SUCCESS);
        }else{
            waitpid(p2, NULL, 0);
            waitpid(p, NULL, 0);
            printf("Waited OK.\n");
        }
    }

    return;

}


void serverCorrectDisconnection(void) {

    int retvalue = 0;
    unsigned long int port = 8083LU;
    struct sockaddr_in server_addr;
    server_addr.sin_port = htons(port);
    server_addr.sin_family = AF_INET;
    char ip[] = "127.0.0.1";
    inet_aton(ip, &server_addr.sin_addr);

    pid_t p = fork();
    if (p == 0) {
        // Son.
        sleep(3);
        int client_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
        printf("Socket created client.\n");
        sleep(2);
        retvalue = connect(client_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        printf("Connected succesfully!\n");
        char x = 'X';
        write(client_fd, &x, 1);
        printf("Message sent by the client: %c!\n", x);
        sleep(5);

        x = '-';
        retvalue = read(client_fd, &x, 1);
        printf("Readed by the client (code: %d): %c.\n", retvalue, x);
        if (retvalue <= 0) {
            printf("Read client error...\n");
            printf("Errno code: %d.\n", (int)errno);
            perror("Errno: ");
        } 
        printf("Client exiting closing fd...\n");
        close(client_fd);
        exit(EXIT_SUCCESS);
    }else{
        // Father.
        pid_t p2;
        p2 = fork();
        if (p2 == 0) {
            // Son.
            int socket_server_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
            printf("Socket created server.\n");
            retvalue = bind(socket_server_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
            printf("Binding complete.\n");
            retvalue = listen(socket_server_fd, SOMAXCONN);
            printf("Listening...\n");
            sleep(4);
            socklen_t l = (socklen_t) sizeof(server_addr);
            int socket_client_fd = accept(socket_server_fd, (struct sockaddr*) (&(server_addr)), &l);
            printf("Client accepted by the server.\n");
            while (1) {
                // Reading a char.
                char x;
                retvalue = read(socket_client_fd, &x, 1);
                printf("Readed by the server (code: %d): %c.\n", retvalue, x);
                break;
            }
            printf("Server exiting closing fd...\n");
            close(socket_client_fd);
            close(socket_server_fd);
            exit(EXIT_SUCCESS);
        }else{
            waitpid(p2, NULL, 0);
            waitpid(p, NULL, 0);
            printf("Waited OK.\n");
        }
    }
}

void clear(void) {
    // Reading a '\n'.
    printf("...Press enter to continue...\n");
    fflush(stdout);
    char x;
    read(STDIN_FILENO, &x, 1);
    system("clear");
    fflush(stdout);
    return;
}

void unhandledData(void) {

    int retvalue = 0;
    unsigned long int port = 8084LU;
    struct sockaddr_in server_addr;
    server_addr.sin_port = htons(port);
    server_addr.sin_family = AF_INET;
    char ip[] = "127.0.0.1";
    inet_aton(ip, &server_addr.sin_addr);

    pid_t p = fork();
    if (p == 0) {
        // Son.
        sleep(3);
        int client_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
        printf("Socket created client.\n");
        sleep(2);
        retvalue = connect(client_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        printf("Connected succesfully!\n");
        char x = 'X';
        write(client_fd, &x, 1);
        printf("Message sent by the client: %c!\n", x);
        sleep(5);
        printf("Client exiting...\n");
        sleep(3);
        exit(EXIT_SUCCESS);
    }else{
        // Father.
        int socket_server_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
        printf("Socket created server.\n");
        retvalue = bind(socket_server_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        printf("Binding complete.\n");
        retvalue = listen(socket_server_fd, SOMAXCONN);
        printf("Listening...\n");
        sleep(4);
        socklen_t l = (socklen_t) sizeof(server_addr);
        int socket_client_fd = accept(socket_server_fd, (struct sockaddr*) (&(server_addr)), &l);
        printf("Client accepted by the server.\n");
        while (1) {
            // Reading a char.
            char x = '-';
            retvalue = read(socket_client_fd, &x, 1);
            printf("Readed by the server (code: %d): %c.\n", retvalue, x);
            if (retvalue <= 0) {
                printf("Errno code: %d.\n", (int)errno);
                perror("Errno: ");
                fflush(stdout);
                break;
            }
            // Reply back, this reply will go to client buffer,
            // but the client will close the socket without reading it.
            char n = 'N';
            retvalue = write(socket_client_fd, &n, 1);
            fflush(stdout);
            sleep(1);
        }
        printf("Server exiting...\n");
        close(socket_client_fd);
        close(socket_server_fd);
        fflush(stdout);
        waitpid(p, NULL, 0);
    }

    return;

}

void noData(void) {

    int retvalue = 0;
    unsigned long int port = 8085LU;
    struct sockaddr_in server_addr;
    server_addr.sin_port = htons(port);
    server_addr.sin_family = AF_INET;
    char ip[] = "127.0.0.1";
    inet_aton(ip, &server_addr.sin_addr);

    pid_t p = fork();
    if (p == 0) {
        // Son.
        sleep(3);
        int client_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
        printf("Socket created client.\n");
        sleep(2);
        retvalue = connect(client_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        printf("Connected succesfully!\n");
        char x = 'X';
        write(client_fd, &x, 1);
        printf("Message sent by the client: %c!\n", x);

        sleep(10);
        retvalue = write(client_fd, &x, 1);
        printf("This should be OK... %d\n", retvalue);
        sleep(5);
        printf("This should trigger EPIPE...\n");
        retvalue = write(client_fd, &x, 1);
        perror("PIPE ");
        printf("ERRNO VALUE: %d. CODE: %d.\n", errno, retvalue);
        sleep(5);
        printf("Client exiting...\n");
        exit(EXIT_SUCCESS);
    }else{
        // Father.
        int socket_server_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
        printf("Socket created server.\n");
        retvalue = bind(socket_server_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        printf("Binding complete.\n");
        retvalue = listen(socket_server_fd, SOMAXCONN);
        printf("Listening...\n");
        sleep(4);
        socklen_t l = (socklen_t) sizeof(server_addr);
        int socket_client_fd = accept(socket_server_fd, (struct sockaddr*) (&(server_addr)), &l);
        printf("Client accepted by the server.\n");
        while (1) {
            // Reading a char.
            char x = '-';
            retvalue = read(socket_client_fd, &x, 1);
            printf("Readed by the server (code: %d): %c.\n", retvalue, x);
            if (retvalue <= 0) {
                printf("Errno code: %d.\n", (int)errno);
                perror("Errno: ");
                fflush(stdout);
                break;
            } 
            break;
            fflush(stdout);
            sleep(1);
        }
        printf("Server exiting...\n");
        close(socket_client_fd);
        //close(socket_server_fd);
        fflush(stdout);
        waitpid(p, NULL, 0);
    }

    return;

}


void sigPIPEHandler(int signum) {

    char str[] = "SIGPIPE HANDLER\n";
    write(STDOUT_FILENO, str, strlen(str));

}


int main(void) {

    struct sigaction sigp; 
    sigp.sa_flags = 0;
    sigp.sa_handler = sigPIPEHandler;   
    int retvalue = sigaction(SIGPIPE, &sigp, NULL);

    // errno == ETIMEDOUT == 60 and retvalue == 0
    system("clear");
    clear();
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
    clear();

    return 0;
    
}




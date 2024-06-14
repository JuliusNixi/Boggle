// Shared client cross files vars and libs.
#include "client.h"

// Current file vars and libs.
#define USAGE_MSG "Invalid args. Usage: ./%s server_ip server_port.\n" // Message to print when the user insert wrong args.

int main(int argc, char** argv) {

    // Initializing local vars.
    int retvalue = 0; // To check system calls result (succes or failure).

    // Initializing shared client cross vars.
    client_fd = 0; 

    // Shared/Common CLIENT & SERVER cross files vars and libs initialization.
    mainthread = pthread_self();

    // Printing banner.
    printf("\n\n##################\n#     CLIENT     #\n##################\n\n");

    // Creating a mask, that will block the SIGINT, SIGALRM and SIGPIPE signals for all 
    // threads except the dedicated thread signalsThread().
    // Important to do it as soon as possible.
    // Not to be caught unprepared.
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGINT);
    sigaddset(&signal_mask, SIGALRM);
    sigaddset(&signal_mask, SIGPIPE);

    // Registering exit function for main.
    retvalue = atexit(atExit);
    if (retvalue != 0) {
        // Error
        // errno
        handleError(1, 1, 0, 1, "Error in registering exit cleanupper main function.\n");
    }
    printff(NULL, "Exit safe function (cleanup for the main) registered correctly.\n");

    // Enabling the mask.
    retvalue = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
    if (retvalue != 0) {
        // Error
        handleError(0, 1, 0, 1, "Error in setting the pthread signals mask.\n");
    }
    printff(NULL, "Threads signals mask enabled correctly.\n");

    struct sigaction sigusr1; // SIGUSR1 sigaction struct, will be used to handle the server's responses.
  
    // SIGUSR1 will NOT be blocked.
    // Setting the checkResponses() handler (does nothing).
    sigusr1.sa_flags = 0;
    sigusr1.sa_handler = checkResponses;   
    retvalue = sigaction(SIGUSR1, &sigusr1, NULL);
    if (retvalue == -1) {
        // Error
        // errno
        handleError(1, 1, 0, 1, "Error in setting SIGUSR1 signal handler.\n");
    }           
    printff(NULL, "SIGUSR1 signal handler registered correctly.\n");

    // Any newly created threads INHERIT the signal mask with these signals blocked. 

    // Creating the thread that will handle ALL AND EXCLUSIVELY SIGINT, SIGALRM and SIGPIPE signals
    // by waiting with sigwait() forever in loop.
    retvalue = pthread_create(&sig_thr_id, NULL, signalsThread, NULL);
    if (retvalue != 0) {
        // Error
        handleError(0, 1, 0, 1, "Error in creating the pthread signals handler.\n");
    }
    printff(NULL, "Signals registered and thread handler started succesfully.\n");

    // Check number of args.
    if (argc != 3) {
        // Error
        handleError(0, 1, 0, 1, USAGE_MSG, argv[0]);
    }

    // Parsing port.
    unsigned long int port = strtoul(argv[2], NULL, 10);
    if (port > 65535LU) {
        // Error
        handleError(0, 1, 0, 1, "Invalid port %lu. It should be less than 65535 and higher than 0.\n", port);
    }
    server_addr.sin_port = htons(port);

    server_addr.sin_family = AF_INET;

    // Parsing IP.
    retvalue = parseIP(argv[1], &server_addr);
    if (retvalue != 1) {
        // Error
        handleError(0, 1, 0, 1, "Invalid IP: %s.\n", argv[1]);
    }
    printf("Starting client on IP: %s and port: %lu.\n", argv[1], port);

    printf("The args seems to be ok...\n");

    // Creating socket.
reconnecting:
    client_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
    if (client_fd == -1) {
        // Error
        handleError(0, 1, 0, 1, "Error in creating socket.\n");
    }
    printf("Socket created.\n");

    // Connecting to server.
    while (1) {
        retvalue = connect(client_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        if (retvalue == -1){
            // On error of connect, the socket will be modified in a unspecified state.
            // So it will not be able to simply trying again a new connect.
            // Read here the notes.
            // https://man7.org/linux/man-pages/man2/connect.2.html
            // Need to recreate the socket...
            // Error
            // errno
            retvalue = close(client_fd);
            if (retvalue != 0) {
                // Error
                // errno
                handleError(1, 1, 0, 1, "Error in socket close(), during the failure of connect().\n");
            }
            printf("Error in connecting, retrying in 3 seconds.\n");
            sleep(3);
            goto reconnecting;
        }else break;
    }
    printf("Connected succesfully!\n");

    // Creating responses handler pthread.
    retvalue = pthread_create(&responses_thread, NULL, responsesHandler, NULL);
    if (retvalue != 0) {
        // Error
        handleError(0, 1, 0, 1, "Error during the responses handler pthread creation.\n");
    }
    printf("Responses pthread created succesfully.\n");

    alarm(CHECK_RESPONSES_SECONDS);
    inputHandler();


    // TODO
    // Exit
/*
    // Chiusura del socket
    SYSC(retvalue, close(client_fd), "nella close");

*/

    return 0;
    
}




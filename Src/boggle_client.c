// Shared client cross files vars and libs.
#include "client.h"

// Current file vars and libs.
#define USAGE_MSG "Invalid args. Usage: ./%s server_ip server_port.\n" // Message to print when the user insert wrong args.

int main(int argc, char** argv) {

    // Printing banner.
    printff(NULL, "\n\n##################\n#     CLIENT     #\n##################\n\n");

    // Initializing local vars.
    int retvalue = 0; // To check system calls result (succes or failure).

    // Initializing shared client cross vars.
    client_fd = 0; 

    // Shared/Common CLIENT & SERVER cross files vars and libs initialization.
    mainthread = pthread_self();
    testmode = 0;

    // To setup the thread destructor.
    threadSetup();

    // Creating a mask, that will block the SIGINT and SIGPIPE signals for all 
    // threads except the dedicated thread signalsThread().
    // Important to do it as soon as possible.
    // Not to be caught unprepared.
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGINT);
    sigaddset(&signal_mask, SIGPIPE);

    // Registering exit function for main.
    retvalue = atexit(atExit);
    if (retvalue != 0) {
        // Error
        handleError(1, 1, 0, 0, "Error in registering exit cleanupper with atexit() in main function.\n");
    }
    printff(NULL, "Exit safe function (cleanup for the main with atexit()) registered correctly.\n");

    // Enabling the mask.
    retvalue = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
    if (retvalue != 0) {
        // Error
        handleError(0, 1, 0, 0, "Error in setting the pthread signals mask.\n");
    }
    printff(NULL, "Threads signals mask enabled correctly.\n");

    // Any newly created threads INHERIT the signal mask with these signals blocked. 

    // Creating the thread that will handle ALL AND EXCLUSIVELY SIGINT and SIGPIPE signals
    // by waiting with sigwait() forever in a loop.
    retvalue = pthread_create(&sig_thr_id, NULL, signalsThread, NULL);
    if (retvalue != 0) {
        // Error
        handleError(0, 1, 0, 0, "Error in creating the pthread signals handler.\n");
    }
    printff(NULL, "Signals registered and pthread handler started succesfully with ID: %lu.\n", (uli) sig_thr_id);


    // Check number of args.
    if (argc != 3) {
        // Error
        handleError(0, 1, 0, 0, USAGE_MSG, argv[0]);
    }

    // Parsing port.
    uli port = strtoul(argv[2], NULL, 10);
    if (port > 65535LU) {
        // Error
        handleError(0, 1, 0, 0, "Invalid port %lu. It should be less than 65535.\n", port);
    }
    server_addr.sin_port = htons(port);

    server_addr.sin_family = AF_INET;

    // Parsing IP.
    retvalue = parseIP(argv[1], &server_addr);
    if (retvalue != 1) {
        // Error
        handleError(0, 1, 0, 0, "Invalid IP: %s.\n", argv[1]);
    }
    printff(NULL, "Starting client on IP: %s and port: %lu.\n", argv[1], port);

    printff(NULL, "The args seems to be ok...\n");

    // Creating socket.
reconnecting:
    client_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
    if (client_fd == -1) {
        // Error
        handleError(0, 1, 0, 0, "Error in creating socket.\n");
    }
    printff(NULL, "Socket created.\n");

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
            retvalue = close(client_fd);
            if (retvalue != 0) {
                // Error
                handleError(1, 1, 0, 0, "Error in socket close(), during the failure of connect().\n");
            }
            printff(NULL, "Error in connecting, retrying in 3 seconds.\n");
            sleep(3);
            goto reconnecting;
        }else break;
    }
    printff(NULL, "Connected succesfully!\n");

    // Creating responses handler pthread.
    retvalue = pthread_create(&responses_thread, NULL, responsesHandler, NULL);
    if (retvalue != 0) {
        // Error
        handleError(0, 1, 0, 0, "Error during the responses handler pthread creation.\n");
    }
    printff(NULL, "Responses pthread created succesfully with ID: %lu.\n", (uli) responses_thread);

    // Start input management.
    inputHandler();


    // TODO Close/Exti socket.



    return 0;
    
}




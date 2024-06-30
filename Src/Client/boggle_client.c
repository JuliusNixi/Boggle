// Shared client cross files vars and libs.
#include "client.h"

// Current file vars and libs.
#define USAGE_MSG "Invalid args. Usage: ./%s server_ip server_port.\n" // Message to print when the user insert wrong args.

int main(int argc, char** argv) {

    // Printing start banner.
    char* banner = bannerCreator(BANNER_LENGTH, BANNER_NSPACES, "SETUP", BANNER_SYMBOL, 0);
    fprintf(stdout, "\n\n##################\n#     CLIENT     #\n##################\n\n");
    fprintf(stdout, "%s\n", banner);
    free(banner);

    // Initializing local vars.
    int retvalue = 0; // To check system calls result (succes or failure).

    // Initializing shared client cross vars.
    client_fd = -1; 

    // Shared/Common CLIENT & SERVER cross files vars and libs initialization.
    mainthread = pthread_self();

    fprintf(stdout, "I'm the main thread (ID): %lu.\n", (uli) mainthread);

    // Creating a mask, that will block the SIGINT and SIGPIPE signals for all 
    // threads except the dedicated thread signalsThread().
    // Important to do it as soon as possible to be able to handle appropriately the signals.
    // Not to be caught unprepared.
    sigemptyset(&signalmask);
    sigaddset(&signalmask, SIGINT);
    sigaddset(&signalmask, SIGPIPE);

    // Enabling the signals' mask.
    retvalue = pthread_sigmask(SIG_BLOCK, &signalmask, NULL);
    if (retvalue != 0) {
        // Error
    }
    fprintf(stdout, "Threads signals mask enabled correctly.\n");

    // Any newly created threads INHERIT the signal mask with these signals blocked. 

    // Creating the pthread that will handle ALL AND EXCLUSIVELY SIGINT and SIGPIPE signals
    // by waiting with sigwait() forever in a loop.
    retvalue = pthread_create(&signalsthread, NULL, signalsThread, NULL);
    if (retvalue != 0) {
        // Error
    }
    fprintf(stdout, "Signals registered and pthread signals handler started succesfully.\n", (uli) signalsthread);

    // Check number of args.
    if (argc != 3) {
        // Error
    }

    // Parsing port.
    uli port = strtoul(argv[2], NULL, 10);
    if (port > 65535LU) {
        // Error
    }
    server_addr.sin_port = htons(port);

    // Socket type.
    server_addr.sin_family = AF_INET;

    // Parsing IP.
    retvalue = parseIP(argv[1], &server_addr);
    if (retvalue != 1) {
        // Error
    }
    fprintf(stdout, "Starting client by connecting to IP: %s and port: %lu.\n", argv[1], port);

    fprintf(stdout, "The args seems to be ok...\n");

    // Creating socket.
reconnecting:
    client_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
    if (client_fd == -1) {
        // Error
    }
    fprintf(stdout, "Socket created.\n");

    // Connecting to server.
    while (1) {
        retvalue = connect(client_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
        if (retvalue == -1){
            // On error of connect, the socket will be modified in a unspecified state.
            // So it will not be able to simply trying again a new connect.
            // Read here the notes.
            // https://man7.org/linux/man-pages/man2/connect.2.html
            // Need to recreate the socket.
            // Error
            retvalue = close(client_fd);
            if (retvalue == -1) {
                // Error
            }
            fprintf(stdout, "Socket closed.\nError in connecting, retrying in 3 seconds.\nIs the server online?\n");
            sleep(3);
            goto reconnecting;
        }else break;
    }
    fprintf(stdout, "Connected succesfully!\n");

    // Creating responses handler pthread.
    retvalue = pthread_create(&responsesthread, NULL, responsesHandler, NULL);
    if (retvalue != 0) {
        // Error
    }
    fprintf(stdout, "Responses pthread created succesfully.\n");

    // Waiting for the setup of other threads.
    char toexit = 0;
    while (1) {
        retvalue = pthread_mutex_lock(&setupmutex);
        if (retvalue != 0) {
            // Error
        }
        if (setupfinished == 2) toexit = 1;
        retvalue = pthread_mutex_unlock(&setupmutex);
        if (retvalue != 0) {
            // Error
        }
        if (toexit) break;
        // To avoid instant reacquiring.
        else usleep(100);
    }

    // Printing end banner.
    banner = bannerCreator(BANNER_LENGTH, BANNER_NSPACES, "END SETUP", BANNER_SYMBOL, 0);
    fprintf(stdout, "%s\n", banner);
    free(banner);

    // Start input management.
    inputHandler();

    // TODO Close socket.

    return 0;
    
}




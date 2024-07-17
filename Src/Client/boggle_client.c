// Shared client cross files vars and libs.
#include "client.h"

// Current file vars and libs.
#define USAGE_MSG "Invalid args. Usage: ./%s server_ip server_port.\n" // Message to print when the user insert wrong args.

int main(int argc, char** argv) {

    // Printing start banner.
    char* banner = bannerCreator(BANNER_LENGTH, BANNER_NSPACES, "SETUP!", BANNER_SYMBOL, 0);
    fprintf(stdout, "\n\n##################\n#     CLIENT     #\n##################\n\n%s\n", banner);
    free(banner);
    banner = NULL;

    // Initializing local vars.
    int retvalue = 0; // To check system calls result (succes or failure).
    char* ip = NULL;

    // Initializing shared client cross vars.
    client_fd = -1; 

    // Shared/Common CLIENT & SERVER cross files vars and libs initialization.
    mainthread = pthread_self();
    sigemptyset(&signalmask);
    setupfinished = 0;
    // Cannot use PTHREAD_MUTEX_INITIALIZER, because can be used only on static allocated mutexes.
    // Initialization should be performed like this.
    retvalue = pthread_mutex_init(&setupmutex, NULL);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error, main() failed to initialize the setupmutex.\n");
        exit(EXIT_FAILURE);
    }
    retvalue = pthread_mutex_init(&printmutex, NULL);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error, main() failed to initialize the printmutex.\n");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "I'm the main thread (ID): %lu.\n", (uli) mainthread);
    pthread_setname_np(mainthread, "MainThread");

    // Creating a mask, that will block the SIGINT and SIGPIPE signals for all 
    // threads except the dedicated thread signalsThread().
    // Important to do it as soon as possible to be able to handle appropriately the signals.
    // Not to be caught unprepared.
    sigaddset(&signalmask, SIGINT);
    sigaddset(&signalmask, SIGPIPE);

    // Enabling the signals' mask.
    retvalue = pthread_sigmask(SIG_BLOCK, &signalmask, NULL);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error, main() failed to set the pthread signals mask.\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "Threads signals mask enabled correctly.\n");

    // Any newly created threads INHERIT the signal mask with these signals blocked. 

    // Creating the pthread that will handle ALL AND EXCLUSIVELY SIGINT and SIGPIPE signals
    // by waiting with sigwait() forever in a loop.
    retvalue = pthread_create(&signalsthread, NULL, signalsThread, NULL);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error, main() failed to create the signalsThread() pthread.\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "Pthread signals handler started succesfully.\n");

    // Check number of args.
    if (argc != 3) {
        // Error
        fprintf(stderr, "Error, invalid number of args.\n");
        fprintf(stderr, USAGE_MSG, argv[0]);
        exit(EXIT_FAILURE);
    }

    // Parsing port.
    uli port = strtoul(argv[2], NULL, 10);
    if (port > 65535LU) {
        // Error
        fprintf(stderr, "Error, invalid port. Must be lower than 65535.\n");
        exit(EXIT_FAILURE);  
    }
    server_addr.sin_port = htons(port);

    // Socket type.
    server_addr.sin_family = AF_INET;

    // Parsing IP.
    retvalue = parseIP(argv[1], &server_addr);
    if (retvalue != 1) {
        // Error
        fprintf(stderr, "Error, invalid IP.\n");
        exit(EXIT_FAILURE); 
    }
    ip = argv[1];
    fprintf(stdout, "Starting client by connecting to IP %s and port %lu.\n", ip, port);

    fprintf(stdout, "The args seems to be ok...\n");

    while (1) {
        // Creating socket.
        client_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
        if (client_fd == -1) {
            // Error
            fprintf(stderr, "Error, bad socket() in main().\n");
            exit(EXIT_FAILURE);
        }

        // Connecting.
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
                fprintf(stderr, "Error, bad close() in main() during the socket restarting.\n");
                exit(EXIT_FAILURE);
            }
            fprintf(stderr, "Socket closed.\nError in connecting, retrying in 3 seconds.\nIs the server online?\n");
            fflush(stderr);
            sleep(3);
            continue;
        }else break;
    }
    fprintf(stdout, "%s\n", CONNECTED_SUCCESFULLY_STR);

    // Creating responses handler pthread.
    retvalue = pthread_create(&responsesthread, NULL, responsesHandler, NULL);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error, main() failed to create the responsesHandler() pthread.\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "Responses pthread created succesfully.\n");

    retvalue = pthread_create(&disconnecterthread, NULL, disconnecterCheckerThread, NULL);
    if (retvalue != 0) {
        // Error
        fprintf(stderr, "Error, main() failed to create the disconnecterCheckerThread() pthread.\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "Disconnect checker pthread started succesfully.\n");

    // Waiting for the setup of other threads.
    char toexit = 0;
    while (1) {
        retvalue = pthread_mutex_lock(&setupmutex);
        if (retvalue != 0) {
            // Error
            fprintf(stderr, "Error, bad setupmutex lock in main().\n");
            exit(EXIT_FAILURE);
        }
        if (setupfinished == 3) toexit = 1;
        retvalue = pthread_mutex_unlock(&setupmutex);
        if (retvalue != 0) {
            // Error
            fprintf(stderr, "Error, bad setupmutex unlock in main().\n");
            exit(EXIT_FAILURE);
        }
        if (toexit) break;
    }

    // Printing end banner.
    banner = bannerCreator(BANNER_LENGTH, BANNER_NSPACES, "SETUP!", BANNER_SYMBOL, 1);
    fprintf(stdout, "%s\n\n", banner);
    free(banner);
    banner = NULL;

    fflush(stdout);

    // Start input management.
    inputHandler();

    // This never executed, but for clarity.
    return 0;
    
}




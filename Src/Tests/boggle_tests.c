// ANCHOR File begin.
#include "../../Src/Common/common.h"
// Needed for waitpid() on Linux.
#include <sys/types.h>
#include <sys/wait.h>

// Remember to compile with also "../../Src/Common/common.c".

// Linux.
// Remember on the LABII machine the maximum open files soft limit (for my user) is 1024.
// Remember each client will use 2 files, 1 to logs stdin and 1 to logs stdout and stderr.
// Difference between soft and hard:
// https://unix.stackexchange.com/questions/29577/ulimit-difference-between-hard-and-soft-limits.
// To see these limits:
// https://stackoverflow.com/questions/1356675/check-the-open-fd-limit-for-a-given-process-in-linux
// macOS.
// I had to increase the limit of open files to run so many clients:
// https://stackoverflow.com/questions/73977844/too-many-open-files-during-rspec-tests-on-macos

// Be particullary moderate when using Valgrind, because it cannot handle high values below.
// These HIGH values are NOT intended to used with Valgrind.
#define N_CLIENTS 64LU // Number of clients that will be spawned.
#define N_ACTIONS 32LU // Number of actions for each client. Each action is the submission of a command.
#define N_TESTS 3LU // Number of tests. It's multiplied by the nactions, be moderate so.

// So each client will submit N_ACTIONS actions for N_TESTS times.

char* actions[] = {"help\n", "matrix\n", "end\n", "register_user", "p", "invalidcommand\n"}; // List of actions that the client will submit from.
uli actionslength = sizeof(actions) / sizeof(actions[0]); // Length of the previous array.
char* finalaction = NULL;

#define USERNAME_LENGTH 4LU // Name length of all names that the clients will try to register.

pid_t pids[N_CLIENTS]; // Array of all the clients processes'PID that will be spawned.
int pipesfdstdin[N_CLIENTS][2]; // Matrix of pipes. Each client will use a pipe to receive input from the father trought STDIN.

// ANCHOR additionalPartialMessageSentTest()
// This cannot be done directly from the spawned client process.
// It's needed to send the server only a partial message and see how it reacts.
// It takes as input the IP of the server, its port and a test id.
void additionalPartialMessageSentTest(char* ip, uli port, uli testnumber) {

     // Parsing port.
    server_addr.sin_port = htons(port);

    // Socket type.
    server_addr.sin_family = AF_INET;
    
    // Parsing IP.
    int retvalue = parseIP(ip, &server_addr);
    if (retvalue != 1) {
        // Error
        fprintf(stderr, "Error, invalid IP.\n");
        exit(EXIT_FAILURE); 
    }

    // Creating socket.
    int client_fd = socket(server_addr.sin_family, SOCK_STREAM, 0);
    if (client_fd == -1) {
        // Error
        fprintf(stderr, "Error, bad socket() in additionalPartialMessageSentTest(), ignoring this test. Trying to continue...\n");
        return;
    }

    // Connecting.
    retvalue = connect(client_fd, (const struct sockaddr*) &server_addr, (socklen_t) sizeof(server_addr));
    if (retvalue == -1){
        retvalue = close(client_fd);
        if (retvalue == -1) {
            // Error
            fprintf(stderr, "Error, additionalPartialMessageSentTest() bad close() during a failed connect().\n");
            exit(EXIT_FAILURE);
        }
        fprintf(stderr, "Socket closed.\nError in connecting in additionalPartialMessageSentTest(), ignoring this test. Trying to continue...\n");
        return;
    }
    
    // Creating an username to register, to distinguish this client test on the server.
    char u[] = "additionalPartialMessageSentTestID%c";
    char um[] = "additionalPartialMessageSentTestID";
    uli l = strlen(um);
    uli n = l + 1 + 1; // +1 for '\0'. +1 for the end char.
    char rst[n];
    sprintf(rst, u, ALPHABET[testnumber]);
    rst[n - 1] = '\0';

    // Registering.
    sendMessage(client_fd, MSG_REGISTRA_UTENTE, rst);

    // Send not the entire message, only a part.
    char type = MSG_REGISTRA_UTENTE;
    retvalue = write(client_fd, &type, sizeof(type));
    if (retvalue == -1) {
        // Error
        fprintf(stderr, "Error in connecting in additionalPartialMessageSentTest(), ignoring this test. Trying to continue...\n");
        return;
    }else if (retvalue == 1) {
        // Ok.
        ;
    }else{
        // Error
        fprintf(stderr, "Error (strnage) in connecting in additionalPartialMessageSentTest().\n");
        exit(EXIT_FAILURE);
    }

    // Important to not close the socket, to test how the server reacts.
    fprintf(stdout, "Test additionalPartialMessageSentTest() sent.\n");
    return;

}

// ANCHOR end()
// This function is used to perform important tasks before exiting the tests.
// It takes as input a char that is used to detect the exiting reason.
void end(char processalive) {

    int retvalue;

    if (processalive == 1)
        fprintf(stdout, "All actions (and all tests) completed.\n");
    else if (processalive == 2){
        // Not all clients connected.
        ;
    }else if (processalive == 3){
        // Error in fork() or executing client's exe.
        ;
    }else if (processalive == 4){
        // Error in opening valid words tests file.
        ;
    }else{
        // Here processalive == 0
        fprintf(stdout, "There's no longer any living client process.\nPress enter to exit...");
        getchar();
        exit(EXIT_SUCCESS);
    }
    fprintf(stdout, "Press enter to kill all the clients processes...");
    getchar();

    // Killing all live client processes.
    for (uli i = 0LU; i < N_CLIENTS; i++) {
        pid_t p = pids[i];

        // Invalid process, skipping it.
        if (p == -1) continue;

        // Checking if the process is alive.
        int status;
        pid_t result = waitpid(p, &status, WNOHANG);
        if (result == 0) {
            // The process is alive, killing it.
            retvalue = kill(p, SIGKILL);
            if (retvalue == -1) {
                // Error
                fprintf(stderr, "Error in killing the %lu client process.\n", i);
                exit(EXIT_FAILURE);
            }
            retvalue = close(pipesfdstdin[i][1]);
            if (retvalue == -1) {
                // Error
                fprintf(stderr, "Error in closing the %lu client process STDIN pipe. Trying to continue...\n", i);
            }else pipesfdstdin[i][1] = -1;
        } else if (result == p) {
            continue; // Process already terminated, skipping.
        }else{
            // Error
            continue; // Process already terminated, skipping.
        }

    } // End for.

    // Waiting processes.
    int status;
    for (uli i = 0LU; i < N_CLIENTS; i++) if(pids[i] != -1) waitpid(pids[i], &status, 0);

    fprintf(stdout, "Killed all clients, exiting...\n");

    if (processalive == 2 || processalive == 3 || processalive == 4) exit(EXIT_FAILURE);

    exit(EXIT_SUCCESS);

}

// ANCHOR client()
// This function is used to spawn a client process by using a fork().
// It takes the file descriptor to the file in wich logs the client process' output (STDOUT and
// STDERR).
// It takes as input the server ip, its port and the pipe to be used to redirect the client
// process' STDIN to the father.
// It returns the pid of the new son process.
pid_t client(int fdstdoutlogfile, char* ip, uli port, int* pipefd) {

    int retvalue;

    // Forking the process.
    pid_t pid;
    pid = fork();
    if (pid == -1) {
        // Error
        // Handled by the father.
        return pid;
    }
    if (pid == 0) {
        // Son.
        // Continue.
        ;
    }else{
        // Return for the father the son's pid.
        return pid;
    }

    // Closing writing pipe.
    retvalue = close(pipefd[1]);
    if (retvalue == -1) {
        // Error
        fprintf(stderr, "Error in client() in closing the writing pipe. Trying to continue...\n");
    }else pipefd[1] = -1;

    // Redirect stdin to pipe.
    retvalue = dup2(pipefd[0], STDIN_FILENO);
    if (retvalue == -1) {
        // Error
        fprintf(stderr, "Error in client() in dup2() on STDIN.\n");
        exit(EXIT_FAILURE);
    }
    retvalue = close(pipefd[0]);
    if (retvalue == -1) {
        // Error
        fprintf(stderr, "Error in client() in closing the pipe. Trying to continue...\n");
    } else pipefd[0] = -1;

    // Redirect STDOUT to logs file.
    retvalue = dup2(fdstdoutlogfile, STDOUT_FILENO);
    if (retvalue == -1) {
        // Error
        fprintf(stderr, "Error in client() in dup2() on STDOUT.\n");
        exit(EXIT_FAILURE);  
    }

    // Redirect STDERR to logs file.
    retvalue = dup2(fdstdoutlogfile, STDERR_FILENO);
    if (retvalue == -1) {
        // Error
        fprintf(stderr, "Error in client() in dup2() on STDERR.\n");
        exit(EXIT_FAILURE);  
    }

    retvalue = close(fdstdoutlogfile);
    if (retvalue == -1) {
        // Error
        fprintf(stderr, "Error in client() in closing the fdstdoutlogfile. Trying to continue...\n");
    }else  fdstdoutlogfile = -1;

    // Getting the port as string.
    char* portstr = itoa(port);
    char portstrstatic[strlen(portstr) + 1]; // +1 for the '\0'.
    strcpy(portstrstatic, portstr);
    portstrstatic[strlen(portstr)] = '\0';
    free(portstr);
    portstr = NULL;
    // Execute client exe.
    execlp("./paroliere_cl", "./paroliere_cl", ip, portstrstatic, NULL);

    // Error in execlp(), handled by the father, returning from this function.
    return -1;

}

int main(int argc, char** argv) {

    // Received args checks.
    if (argc != 3){
        // Error
        fprintf(stderr, "Invalid args. Pass the IP and the port to be used for the clients connection (to...).\n");
        exit(EXIT_FAILURE);
    }

    // Setting random seed.
    srand(RAND_SEED);

    // Parsing port.
    uli port = strtoul(argv[2], NULL, 10);
    if (port > 65535LU) {
        // Error
    }
    
    // Parsing IP.
    int retvalue = parseIP(argv[1], &server_addr);
    if (retvalue != 1) {
        // Error
        fprintf(stderr, "Error, invalid port. Must be lower than 65535.\n");
        exit(EXIT_FAILURE);
    }
    char* ip = argv[1];

    // Some prints.
    fprintf(stdout, "Remember to start this program with current folder as the project's root '/'.\n");
    fprintf(stdout, "You should open the server on IP %s and port %lu.\n", ip, port);
    fprintf(stdout, "Clients: %lu. Actions: %lu. Tests %lu.\n", N_CLIENTS, N_ACTIONS, N_TESTS);
    fprintf(stdout, "When you're ready, press enter to start the tests...");
    fflush(stdout);
    getchar();

    fprintf(stdout, "Starting clients...\n");
    // Initializing pids client processes array.
    for (uli i = 0LU; i < N_CLIENTS; i++)
        pids[i] = -1;
    // Creating clients processes.
    for (uli i = 0LU; i < N_CLIENTS; i++) {
        // Creating the STDOUT and STDERR logs file path.
        char* istr = itoa(i);
        uli li = strlen(istr);
        char strpathout[] = "./Tests/C/Logs/stdout-log-%lu.txt";
        char strpathoutm[] = "./Tests/C/Logs/stdout-log-.txt";
        uli l = strlen(strpathoutm);
        uli n = l + li + 1; // +1 for the '\0'.
        char rstrout[n];
        sprintf(rstrout, strpathout, i);
        rstrout[n - 1] = '\0';
        free(istr);
        istr = NULL;

        // Opening the STDOUT and STDERR logs file.
        int fd = open(rstrout, O_TRUNC | O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); // Creating the stdout logs file.
        if (fd == -1) {
            // Error
            fprintf(stderr, "Error in creating the STDOUT and STDERR logs file.\n");
            exit(EXIT_FAILURE);
        }

        // Create a pipe to communicate with the client process' STDIN.
        if (pipe(pipesfdstdin[i]) == -1) {
            // Error
            fprintf(stderr, "Error in creating the pipe to be used to connect the STDIN of the client process.\n");
            exit(EXIT_FAILURE);
        }

        // Fork inside the function and son's pid returned.
        pid_t pid = client(fd, argv[1], port, pipesfdstdin[i]);
        
        // Father.
        if (pid == -1) {
            // Error
            fprintf(stderr, "Error in fork() or executing client's exe.\n");
            end(3);
        }

        // Saving pid in the array.
        pids[i] = pid;

        retvalue = close(fd);
        if (retvalue == -1) {
            // Error
            fprintf(stderr, "Error in closing the STDOUT and STDERR logs file from the father. Trying to continue...\n");
        }

        // Closing reading pipe.
        retvalue = close(pipesfdstdin[i][0]);
        if (retvalue == -1) {
            // Error
            fprintf(stderr, "Error in closing the STDOUT and STDERR logs file from the father. Trying to continue...\n");
        }else pipesfdstdin[i][0] = -1;

        // Creating the STDIN logs file path.
        char strpathin[] = "./Tests/C/Logs/stdin-log-%lu.txt";
        char strpathinm[] = "./Tests/C/Logs/stdin-log-.txt";
        l = strlen(strpathinm);
        n = l + li + 1;
        char rstrin[n];
        sprintf(rstrin, strpathin, i);
        strpathin[n - 1] = '\0';

        // Creating the STDIN logs file.
        fd = open(rstrin, O_TRUNC | O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); // Creating the stdin logs file.
        if (fd == -1) {
            // Error
            fprintf(stderr, "Error in creating the STDIN logs file.\n");
            exit(EXIT_FAILURE);
        }
        retvalue = close(fd);
        if (retvalue == -1) {
            // Error
            fprintf(stderr, "Error in closing the STDIN logs file from the father. Trying to continue...\n");
        } else fd = -1;

        fprintf(stdout, "Client %lu spawned.\n", i);
        fflush(stdout);
        // Sleep 0.1 seconds.
        usleep(100000);

    }
    fprintf(stdout, "All clients opened.\nPress enter to continue...");

    getchar();

    // Checking if all clients are connected succesfully to the server.
    fprintf(stdout, "Checking if all clients are connected...\n");
    for (uli i = 0LU; i < N_CLIENTS; i++){

        // Creating the STDOUT and STDERR logs file path.
        char* istr = itoa(i);
        uli li = strlen(istr);
        char strpathout[] = "./Tests/C/Logs/stdout-log-%lu.txt";
        char strpathoutm[] = "./Tests/C/Logs/stdout-log-.txt";
        uli l = strlen(strpathoutm);
        uli n = l + li + 1;
        char rstrout[n];
        sprintf(rstrout, strpathout, i);
        rstrout[n - 1] = '\0';
        free(istr);
        istr = NULL;

        // Performing stat on file.
        struct stat s;
        retvalue = stat(rstrout, &s);
        if (retvalue == -1) {
            // Error
            fprintf(stderr, "Error in stat() during the clients connections check.\n");
            exit(EXIT_FAILURE);
        }

        // To store file content.
        // Total size, in bytes + 1 for the '\0'. 
        char file[s.st_size + 1];
        char file_copy[s.st_size + 1];

        // Opening the STDOUT and STDERR logs file.
        FILE* stdoutfile = fopen(rstrout, "r");
        if (stdoutfile == NULL) {
            // Error
            fprintf(stderr, "Error in fopen() during the clients connections check.\n");
            exit(EXIT_FAILURE); 
        }

        // Reading the STDOUT and STDERR logs file.
        retvalue = fread(file, sizeof(char), s.st_size, stdoutfile);
        if (retvalue != s.st_size) {
            // Error
            fprintf(stderr, "Error in fread() during the clients connections check.\n");
            exit(EXIT_FAILURE);
        }

        // Terminating the file content.
        file[s.st_size] = '\0';
        // Copying the file content, to use strtok() on the first instance.
        strcpy(file_copy, file);
        file_copy[s.st_size] = '\0';

        // Counting file lines.
        char* str = file;
        uli counter = 0LU;
        while (str != NULL) {
            // strtok() modifies the string content.
            // It tokenize all '\n' '\r' '\n\r' '\r\n'.
            // This tokens are replaced with '\0'.
            // The first time strtok() need to be called with string pointer, then with NULL.
            if (counter == 0LU) str = strtok(str, "\n\r");
            else str = strtok(NULL, "\n\r");
            counter++;
        }

        uli arraylen = --counter;
        char* lines[arraylen];

        // Copying each (line) of the file.
        counter = 0LU;
        str = file_copy;
        while (str != NULL) {
            // Totkenizing with strtok().
            if (counter == 0LU) str = strtok(str, "\n\r");
            else str = strtok(NULL, "\n\r");

            if (str == NULL) break;

            // Allocating heap space.
            lines[counter++] = (char*) malloc(sizeof(char) * (strlen(str) + 1));
            if (lines[counter - 1] == NULL) {
                // Error
                fprintf(stderr, "Error in malloc() for the STDOUT file lines during the clients connections check.\n");
                exit(EXIT_FAILURE);
            }

            // Copying the word in the new words[i] heap space.
            strcpy(lines[counter - 1], str);
            lines[counter - 1][strlen(str)] = '\0';

        }

        // Cheking if in the STDOUT and STDERR logs file there is the confirmation of the
        // connection to the server.
        char found = 0;
        for (uli j = 0LU; j < arraylen; j++) {
            if (strcmp(lines[j], CONNECTED_SUCCESFULLY_STR) == 0) found = 1;
            if (found) break;
        }
        if (!found) {
            // Error
            fprintf(stderr, "The client %lu failed to connect to the server!\n", i);
            end(2);
        }
        // Freeing memory.
        for (uli j = 0LU; j < arraylen; j++) {
            free(lines[j]);
            lines[j] = NULL;
        }
        // No need to free lines, it's static.

    }
    fprintf(stdout, "All clients connected to the server succesfully!\nPress enter to continue...");

    getchar();

    // Starting submitting actions to the server.
    fprintf(stdout, "Starting actions tests...\n");
    // ANCHOR Tests
    for (uli t = 0LU; t < N_TESTS; t++){
        fprintf(stdout, "Test %lu.\n", t);
        fflush(stdout);
        
        // Special partial message test.
        additionalPartialMessageSentTest(ip, port, t);

        // ANCHOR Actions
        for (uli a = 0LU; a < N_ACTIONS; a++) {
            fprintf(stdout, "Action %lu.\n", a);

            // Sleeping between every action some random time.
            // Sleep random from 2 to 6 seconds.
            int randint = rand() % 5;
            if (randint == 0) randint++;
            randint += 1;
            usleep(((double)randint / 10.0) * 10000000);

            // Use this flag to see if all process are terminated or there is at least one alive.
            // To exit prematurely.
            char someonealiveflag = 0;

            // ANCHOR Clients
            for (uli i = 0LU; i < N_CLIENTS; i++) {

                // Sleep 0.01 seconds.
                usleep(10000);

                pid_t p = pids[i];

                // Checking if the client process is alive.
                int status;
                pid_t result = waitpid(p, &status, WNOHANG);
                if (result == 0) {
                    someonealiveflag = 1; // Process alive.
                } else if (result == p) {
                    continue; // Process terminated, skipping.
                }else{
                    // -1.
                    // Exited process ("end").
                    // Skipping.
                    continue;
                }

                // Creating the STDIN logs file path.
                char* istr = itoa(i);
                uli li = strlen(istr);
                free(istr);
                istr = NULL;
                char strpathin[] = "./Tests/C/Logs/stdin-log-%lu.txt";
                char strpathinm[] = "./Tests/C/Logs/stdin-log-.txt";
                uli l = strlen(strpathinm);
                uli n = l + li + 1;
                char rstrin[n];
                sprintf(rstrin, strpathin, i);
                strpathin[n - 1] = '\0';

                // Opening the STDIN logs file.
                int filestdinlogsfd = open(rstrin, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); // Appending the stdin logs file.
                if (filestdinlogsfd == -1) {
                    // Error
                    fprintf(stderr, "Error in submitting action, in opening the STDIN logs file.\n");
                    exit(EXIT_FAILURE);
                }

                // Submitting a void action (do nothing) if randint >= 95.
                randint = rand() % 101;
                if (randint >= 95) {
                    // Logging it.
                    retvalue = write(filestdinlogsfd, "nothing\n", strlen("nothing\n"));
                    if (retvalue == -1) {
                        // Error
                        fprintf(stderr, "Error in submitting, writing 'nothing' action.\n");
                        exit(EXIT_FAILURE);
                    }
                    retvalue = close(filestdinlogsfd);
                    if (retvalue == -1) {
                        // Error
                        fprintf(stderr, "Error in submitting, writing 'nothing' action, in closing the STDIN logs file.\n");
                        exit(EXIT_FAILURE);
                    }else filestdinlogsfd = -1;
                    continue;
                }

                // Chosing a random action from the above list.
                randint = rand() % actionslength;
                int choosenaction = randint;
                char* action = actions[randint];

                finalaction = NULL;

                // Register user action.
                if (action == actions[3]) { // register_user
                    char user[USERNAME_LENGTH + 1]; // +1 for '\0'. 
                    // Creating a random username.
                    for (uli j = 0LU; j < USERNAME_LENGTH; j++) {
                        randint = rand() % strlen(ALPHABET);
                        user[j] = ALPHABET[randint];
                    }
                    // Submitting an invalid username if randint >= 80.
                    randint = rand() % 101;
                    if (randint >= 80)
                        user[0] = VOID_CHAR;
                    user[USERNAME_LENGTH] = '\0';
                    uli l = strlen(action) + 1 /* For space */ + USERNAME_LENGTH + 1 /* For '\n' */
                    + 1; // For '\0'.
                    finalaction = (char*) malloc(sizeof(char) * l);
                    if (finalaction == NULL) {
                        // Error
                        fprintf(stderr, "Error in submitting, preparing 'register_user' action, in malloc() for the action.\n");
                        exit(EXIT_FAILURE);
                    }
                    uli counter = 0LU;
                    while (1) {
                        if (action[counter] == '\0') break;
                        finalaction[counter] = action[counter];
                        counter++;
                    }
                    finalaction[counter++] = ' ';
                    uli counter2 = 0LU;
                    while (1) {
                        if (user[counter2] == '\0') break;
                        finalaction[counter] = user[counter2];
                        counter++;
                        counter2++;
                    }
                    finalaction[counter++] = '\n';
                    finalaction[counter] = '\0';
                    action = finalaction;
                } // End if register_user.

                // Submit word action.
                if (action == actions[4]){ // p
                    // Reading current valid words file.
                    // Performing stat on file.
                    struct stat s;
                    retvalue = stat(VALID_WORDS_TESTS_FILE_PATH, &s);
                    if (retvalue == -1) {
                        // Error
                        fprintf(stderr, "Error in submitting, preparing 'p' action, in stat().\n");
                        exit(EXIT_FAILURE);          
                    }

                    // Opening the file in readonly mode.
                    int fd = open(VALID_WORDS_TESTS_FILE_PATH, O_RDONLY, NULL);
                    if (fd == -1) {
                        // Error
                        fprintf(stderr, "Error in opening valid words tests file.\n");
                        end(4);
                    }

                    char file[s.st_size + 1];
                    char file_copy[s.st_size + 1];
                    uli counter = 0LU;
                    // Reading the file content using a buffer of BUFFER_SIZE length.
                    char buffer[BUFFER_SIZE];
                    while (1) {
                        retvalue = read(fd, buffer, BUFFER_SIZE);
                        if (retvalue == -1) {
                            // Error
                            fprintf(stderr, "Error in submitting, preparing 'p' action, in reading VALID_WORDS_TESTS_FILE_PATH file.\n");
                            exit(EXIT_FAILURE);  
                        }

                        // Exit while, end of file reached.
                        if (retvalue == 0) break;

                        // Copying the buffer in the main file array.
                        for (uli j = 0LU; j < retvalue; j++)
                            file[counter++] = buffer[j];
                    }
                    file[counter] = '\0';

                    retvalue = close(fd);
                    if (retvalue == -1) {
                        // Error
                        fprintf(stderr, "Error in submitting, preparing 'p' action, in closing VALID_WORDS_TESTS_FILE_PATH file.\n");
                        exit(EXIT_FAILURE);  
                    }else fd = -1;

                    // Copying the file content, to use strtok() on the first instance.
                    strcpy(file_copy, file);
                    file_copy[s.st_size] = '\0';

                    // Counting file lines and allocating heap space.
                    char* str = file;
                    counter = 0LU;
                    while (str != NULL) {
                        // strtok() modifies the string content.
                        // It tokenize all '\n' '\r' '\n\r' '\r\n'.
                        // This tokens are replaced with '\0'.
                        // The first time strtok() need to be called with string pointer, then with NULL.
                        if (counter == 0LU) str = strtok(str, "\n\r");
                        else str = strtok(NULL, "\n\r");
                        counter++;
                    }

                    uli words_len = --counter;

                    // No valid words.
                    if (words_len == 0LU) {
                        // Logging it.
                        retvalue = write(filestdinlogsfd, "no valid words\n", strlen("no valid words\n"));
                        if (retvalue == -1) {
                            // Error
                            fprintf(stderr, "Error in submitting, preparing 'p' action, no valid words, in write() to the STDIN logs file.\n");
                            exit(EXIT_FAILURE);  
                        }
                        retvalue = close(filestdinlogsfd);
                        if (retvalue == -1) {
                            // Error
                            fprintf(stderr, "Error in submitting, preparing 'p' action, no valid words, in closing() the STDIN logs file.\n");
                            exit(EXIT_FAILURE); 
                        }else filestdinlogsfd = -1;
                        continue;
                    }

                    char* words[words_len];

                    // Copying each word (line) of the file in "words[i]".
                    counter = 0LU;
                    str = file_copy;
                    while (str != NULL) {
                        // Totkenizing with strtok().
                        if (counter == 0LU) str = strtok(str, "\n\r");
                        else str = strtok(NULL, "\n\r");

                        if (str == NULL) break;

                        // Allocating heap space.
                        words[counter++] = (char*) malloc(sizeof(char) * (strlen(str) + 1));
                        if (words[counter - 1] == NULL) {
                            // Error
                            fprintf(stderr, "Error in submitting, preparing 'p' action, in malloc() for a word.\n");
                            exit(EXIT_FAILURE);
                        }

                        // Copying the word in the new words[i] heap space.
                        strcpy(words[counter - 1], str);
                        words[counter - 1][strlen(str)] = '\0';
                    }

                    // Choosing a random valid word.
                    char* word;
                    randint = rand() % words_len;
                    word = words[randint];

                    // Submitting an invalid word if randint >= 50.
                    randint = rand() % 101;
                    if (randint >= 50)
                        word[0] = VOID_CHAR;

                    uli l = strlen(action) + 1 /* For space */ + strlen(word) + 1 /* For '\n' */
                    + 1; // For '\0'.
                    finalaction = (char*) malloc(sizeof(char) * l);
                    if (finalaction == NULL) {
                        // Error
                        fprintf(stderr, "Error in submitting, preparing 'p' action, in malloc() for the action.\n");
                        exit(EXIT_FAILURE);
                    }
                    counter = 0LU;
                    while (1) {
                        if (action[counter] == '\0') break;
                        finalaction[counter] = action[counter];
                        counter++;
                    }
                    finalaction[counter++] = ' ';
                    uli counter2 = 0LU;
                    while (1) {
                        if (word[counter2] == '\0') break;
                        finalaction[counter] = word[counter2];
                        counter++;
                        counter2++;
                    }
                    finalaction[counter++] = '\n';
                    finalaction[counter] = '\0';
                    action = finalaction;   

                    // Freeing memory.
                    for (uli z = 0LU; z < words_len; z++) {
                        free(words[z]);
                        words[z] = NULL;
                    }

                } // End p.

                // Correcting too frequent "end" action.
                if (choosenaction == 2){
                    randint = rand() % 101;
                    if (randint <= 95){
                        // Logging action.
                        retvalue = write(filestdinlogsfd, "skippingend\n", strlen("skippingend\n"));
                        if (retvalue == -1) {
                            // Error
                            fprintf(stderr, "Error in correcting 'p' action, in write() to the STDIN logs file.\n");
                            exit(EXIT_FAILURE);  
                        }
                        retvalue = close(filestdinlogsfd);
                        if (retvalue == -1) {
                            // Error
                            fprintf(stderr, "Error in correcting 'p' action, in closing the STDIN logs file.\n");
                            exit(EXIT_FAILURE);
                        }else filestdinlogsfd = -1;
                        free(finalaction);
                        finalaction = NULL;
                        continue;
                    }
                }

                // Submitting action.
                // Submitting.
                if (pipesfdstdin[i][1] != -1) {
                    retvalue = write(pipesfdstdin[i][1], action, strlen(action));
                    if (retvalue == -1) {
                        // Error
                        fprintf(stderr, "Error in submitting, writing action in the pipe.\n");
                        exit(EXIT_FAILURE);
                    }
                }else{
                    retvalue = close(filestdinlogsfd);
                    if (retvalue == -1) {
                        // Error
                        fprintf(stderr, "Error in submitting, closing the STDIN logs file.\n");
                        exit(EXIT_FAILURE);
                    } else filestdinlogsfd = -1;
                    free(finalaction);
                    finalaction = NULL;
                    continue;
                } 

                // Closing pipe on "end".
                if (choosenaction == 2){
                    if (pipesfdstdin[i][1] != -1) {
                        retvalue = close(pipesfdstdin[i][1]);
                        if (retvalue == -1) {
                            // Error
                        fprintf(stderr, "Error in submitting, end, closing the pipe.\n");
                        exit(EXIT_FAILURE);
                        }else pipesfdstdin[i][1] = -1;
                    }
                }

                // Logging action.
                retvalue = write(filestdinlogsfd, action, strlen(action));
                if (retvalue == -1) {
                    // Error
                    fprintf(stderr, "Error in submitting, logging the action in the file.\n");
                    exit(EXIT_FAILURE);
                }

                // SIGINT the client if r >= 99.
                randint = rand() % 101;
                if (randint >= 99 && choosenaction != 2){
                    retvalue = write(filestdinlogsfd, "sigint\n", strlen("sigint\n"));
                    if (retvalue == -1) {
                        // Error
                        fprintf(stderr, "Error in SIGINT, logging the action in the file.\n");
                        exit(EXIT_FAILURE);
                    }              
                    retvalue = kill(pids[i], SIGINT);
                    if (retvalue == -1) {
                        // Error
                        fprintf(stderr, "Error in SIGINT, killing.\n");
                        exit(EXIT_FAILURE);
                    }
                    retvalue = close(pipesfdstdin[i][1]);
                      if (retvalue == -1) {
                        // Error
                        fprintf(stderr, "Error in SIGINT, closing the STDIN logs file.\n");
                        exit(EXIT_FAILURE);
                    }else pipesfdstdin[i][1] = -1;              
                }

                retvalue = close(filestdinlogsfd);
                if (retvalue == -1) {
                    // Error
                    fprintf(stderr, "Error in submitting, closing the STDIN logs file.\n");
                    exit(EXIT_FAILURE);
                }else filestdinlogsfd = -1;

                free(finalaction);
                finalaction = NULL;

            } // End for clients.

            // If no client process is alive exiting prematurely.
            if (someonealiveflag == 0) end(0);

        } // End for actions.

    } // End for tests.

    // Normal exit, all tests completed and still clients processes alive.
    end(1);

    return 0;

}



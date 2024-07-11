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
#define N_CLIENTS 256LU // Number of clients that will be spawned.
#define N_ACTIONS 32LU // Number of actions for each client. Each action is the submission of a command.
#define N_TESTS 8LU // Number of tests. It's multiplied by the nactions, be moderate so.

char* actions[] = {"help\n", "matrix\n", "enddisabled\n", "register_user", "p", "invalidcommand\n"};
#define ACTIONS_LENGTH 6LU
char* finalaction = NULL;

#define USERNAME_LENGTH 4LU // Name length of all names that the clients will try to register.

pid_t pids[N_CLIENTS];
int pipesfdstdin[N_CLIENTS][2];
int stdoutfds[N_CLIENTS];

void end(char processalive) {

    int retvalue;

    if (processalive)
        fprintf(stdout, "All actions (and all tests) completed.\n");
    else{
        fprintf(stdout, "There's no longer any living client process.\n");
        exit(0);
    }
    fprintf(stdout, "Press enter to kill all the clients processes...");
    getchar();

    for (uli i = 0LU; i < N_CLIENTS; i++) {
        pid_t p = pids[i];

        int status;
        pid_t result = waitpid(p, &status, WNOHANG);
        if (result == 0) {
            // The process is alive, killing it.
            retvalue = kill(p, SIGKILL);
            if (retvalue == -1) {
                // Error
            }
            retvalue = close(stdoutfds[i]);
            if (retvalue == -1) {
                // Error
            }
            retvalue = close(pipesfdstdin[i][1]);
            if (retvalue == -1) {
                // Error
            }
        } else if (result == p) {
            continue; // Process terminated, skipping.
        }else{
            // Error
        }
    }

    int status;
    for (uli i = 0LU; i < N_CLIENTS; i++) waitpid(pids[i], &status, 0);

    fprintf(stdout, "Killed all clients, exiting...\n");
    exit(0);

}

// ANCHOR client()
pid_t client(int fdstdoutlogfile, char* ip, uli port, int* pipefd) {

    int retvalue;

    pid_t pid;
    pid = fork();
    if (pid == -1) {
        // Error
fprintf(stderr,"\n\n error 1 \n\n");
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
fprintf(stderr,"\n\n error a \n\n");
    }

    // Redirect stdin to pipe.
    retvalue = dup2(pipefd[0], STDIN_FILENO);
    if (retvalue == -1) {
        // Error
fprintf(stderr,"\n\n error b \n\n");

    }
    retvalue = close(pipefd[0]);
    if (retvalue == -1) {
        // Error
fprintf(stderr,"\n\n error c \n\n");
    }

    // Redirect stdout to logs file.
    retvalue = dup2(fdstdoutlogfile, STDOUT_FILENO);
    if (retvalue == -1) {
        // Error
fprintf(stderr,"\n\n error d \n\n");
    }

    // Redirect stderr to logs file.
    retvalue = dup2(fdstdoutlogfile, STDERR_FILENO);
    if (retvalue == -1) {
        // Error
fprintf(stderr,"\n\n error e \n\n");
    }

    retvalue = close(fdstdoutlogfile);
    if (retvalue == -1) {
        // Error
fprintf(stderr,"\n\n error f \n\n");
    }

    // Execute client.
    char* portstr = itoa(port);
    char portstrstatic[strlen(portstr) + 1];
    strcpy(portstrstatic, portstr);
    portstrstatic[strlen(portstr)] = '\0';
    free(portstr);
    portstr = NULL;
    execlp("./paroliere_cl", "./paroliere_cl", ip, portstrstatic, NULL);

    // Error
    return -1;

}

int main(int argc, char** argv) {

    // Clearing the screen.
    system("clear");

    // Received args checks.
    if (argc != 3){
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
    }

    fprintf(stdout, "Remember to start this program with current folder as the project's root '/'.\n");
    fprintf(stdout, "You should open the server on IP: %s and port: %lu.\n", argv[1], port);
    fprintf(stdout, "Clients: %lu. Actions: %lu. Tests %lu.\n", N_CLIENTS, N_ACTIONS, N_TESTS);
    fprintf(stdout, "When you're ready, press enter to start the tests...");
    getchar();

    fprintf(stdout, "Starting clients...\n");
    for (uli i = 0LU; i < N_CLIENTS; i++) {
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
        int fd = open(rstrout, O_TRUNC | O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); // Creating the stdout logs file.
        if (fd == -1) {
            // Error
        }
        stdoutfds[i] = fd;
        // Create a pipe.
        if (pipe(pipesfdstdin[i]) == -1) {
            // Error
        }
        pid_t pid = client(fd, argv[1], port, pipesfdstdin[i]);
        if (pid == -1) {
            // Error
            fprintf(stderr, "Error in executing client's process.\n");
            exit(1);
        }
        // Father.
        // Closing reading pipe.
        close(pipesfdstdin[i][0]);
        pids[i] = pid;
        char strpathin[] = "./Tests/C/Logs/stdin-log-%lu.txt";
        char strpathinm[] = "./Tests/C/Logs/stdin-log-.txt";
        l = strlen(strpathinm);
        n = l + li + 1;
        char rstrin[n];
        sprintf(rstrin, strpathin, i);
        strpathin[n - 1] = '\0';
        fd = open(rstrin, O_TRUNC | O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); // Creating the stdin logs file.
        if (fd == -1) {
            // Error
        }
        retvalue = close(fd);
        if (retvalue == -1) {
            // Error
        }
    }
    fprintf(stdout, "All clients opened.\n");

    sleep(3);

    fprintf(stdout, "Starting actions tests...\n");
    for (uli t = 0LU; t < N_TESTS; t++){
        fprintf(stdout, "Test %lu.\n", t);
        for (uli a = 0LU; a < N_ACTIONS; a++) {
            // Sleeping between every action some random time.
            int randint = rand() % 11;
            usleep(randint * 10);

            // Use this flag to see if all process are terminated or there is at least one alive.
            char someonealiveflag = 0;

            for (uli i = 0LU; i < N_CLIENTS; i++) {
                pid_t p = pids[i];

                int status;
                pid_t result = waitpid(p, &status, WNOHANG);
                if (result == 0) {
                    someonealiveflag = 1; // Process alive.
                } else if (result == p) {
                    continue; // Process terminated, skipping.
                }else{
                    // Error
                }

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
                int filestdinlogsfd = open(rstrin, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); // Appending the stdin logs file.
                if (filestdinlogsfd == -1) {
                    // Error
                }

                // Submitting a void action (do nothing) if randint >= 95.
                randint = rand() % 101;
                if (randint >= 95) {
                    // Logging it.
                    retvalue = write(filestdinlogsfd, "nothing\n", strlen("nothing\n"));
                    if (retvalue == -1) {
                        // Error
                    }
                    retvalue = close(filestdinlogsfd);
                    if (retvalue == -1) {
                        // Error
                    }
                    continue;
                }

                // Chosing a random action from the above list.
                randint = rand() % ACTIONS_LENGTH;
                char* action = actions[randint];

                finalaction = NULL;

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
                        user[USERNAME_LENGTH - 1] = VOID_CHAR;
                    user[USERNAME_LENGTH] = '\0';
                    uli l = strlen(action) + 1 /* For space */ + USERNAME_LENGTH + 1 /* For '\n' */
                    + 1; // +1 for '\0'.
                    finalaction = (char*) malloc(sizeof(char) * l);
                    if (finalaction == NULL) {
                        // Error
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
                }

                if (action == actions[4]){ // p
                    // Reading current valid words file.
                    // Performing stat on file.
                    struct stat s;
                    retvalue = stat(VALID_WORDS_TESTS_FILE_PATH, &s);
                    if (retvalue == -1) {
                        // Error
                    }
                    // Check if the file is regular.
                    if(!S_ISREG(s.st_mode)){
                        // Error
                    }
                    // Opening the file in readonly mode.
                    int fd = open(VALID_WORDS_TESTS_FILE_PATH, O_RDONLY, NULL);
                    if (fd == -1) {
                        // Error
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
                    }
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
                    // Allocating heap spaces for "words" and setting "words_len".
                    uli words_len = --counter;
                    // No valid words.
                    if (words_len == 0LU) {
                        // Logging it.
                        retvalue = write(filestdinlogsfd, "no valid words\n", strlen("no valid words\n"));
                        if (retvalue == -1) {
                            // Error
                        }
                        retvalue = close(filestdinlogsfd);
                        if (retvalue == -1) {
                            // Error
                        }
                        continue;
                    }
                    char** words = (char**) malloc(sizeof(char*) * words_len);
                    if (words == NULL) {
                        // Error
                    }

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
                        }

                        // Copying the word in the new words[i] heap space.
                        strcpy(words[counter - 1], str);
                        words[counter - 1][strlen(words[counter - 1])] = '\0';
                    }
                    char* word;

                    randint = rand() % words_len;
                    word = words[randint];

                    // Submitting an invalid word if randint >= 50.
                    randint = rand() % 101;
                    if (randint >= 50)
                        word[0] = VOID_CHAR;

                    uli l = strlen(action) + 1 /* For space */ + strlen(word) + 1 /* For '\n' */
                    + 1; // +1 for '\0'.
                    finalaction = (char*) malloc(sizeof(char) * l);
                    if (finalaction == NULL) {
                        // Error
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

                    for (uli z = 0LU; z < words_len; z++) {
                        free(words[z]);
                        words[z] = NULL;
                    }
                    free(words);
                    words = NULL;
                } // End p.

                // Submitting action.
                // Submitting.
                retvalue = write(pipesfdstdin[i][1], action, strlen(action));
                if (retvalue == -1) {
                    // Error
                }

                if (action == actions[2]){
                    retvalue = close(stdoutfds[i]);
                    if (retvalue == -1) {
                        // Error
                    }
                    retvalue = close(pipesfdstdin[i][1]);
                    if (retvalue == -1) {
                        // Error
                    }
                }

                // Logging action.
                retvalue = write(filestdinlogsfd, action, strlen(action));
                if (retvalue == -1) {
                    // Error
                }

                // Killing the client if r >= 99.
                randint = rand() % 101;
                if (randint >= 99){
                    retvalue = write(filestdinlogsfd, "sigint\n", strlen("sigint\n"));
                    if (retvalue == -1) {
                        // Error
                    }              
                    retvalue = kill(pids[i], SIGINT);
                    if (retvalue == -1) {
                        // Error
                    }
                    retvalue = close(stdoutfds[i]);
                    if (retvalue == -1) {
                        // Error
                    }
                    retvalue = close(pipesfdstdin[i][1]);
                      if (retvalue == -1) {
                        // Error
                    }                  
                }

                retvalue = close(filestdinlogsfd);
                if (retvalue == -1) {
                    // Error
                }

                free(finalaction);
                finalaction = NULL;

            } // End for clients.

            if (someonealiveflag == 0) end(0);

        } // End for actions.


    } // End for tests.

    end(1);

    return 0;

}


/*
     
        






*/


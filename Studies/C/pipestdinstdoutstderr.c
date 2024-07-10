#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define BUFFER_SIZE 256

int main(void) {

    int pipefd[2];
    pid_t pid;

    // Create a pipe.
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Create a new process.
    pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Son's code.

        char buffer[BUFFER_SIZE];
        // Closing writing pipe.
        close(pipefd[1]);  

        // Redirect son's stdin to pipe.
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);

        // Open logs file.
        int logfd = open("./logs.txt", O_TRUNC | O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); // Creating the stdout logs file.
        if (logfd == -1) {
            perror("open logs file");
            exit(EXIT_FAILURE);
        }

        // Redirect stdout to logs file.
        dup2(logfd, STDOUT_FILENO);
        // Redirect stderr to logs file.
        dup2(logfd, STDERR_FILENO);
        close(logfd);

        // Reading from stdin (now attached to pipe).
        fgets(buffer, BUFFER_SIZE, stdin);
        printf("The son process received: \"%s\".\n", buffer);
        fflush(stdout);

        // Testing stderr.
        fprintf(stderr, "Testing stderr...\n");
        fflush(stderr);

        // Execute "ls" with the arg received from stdin.
        execlp("ls", "ls", buffer, NULL);

        // Error


    } else {
        // Father's code.

        // Closing reading pipe.
        close(pipefd[0]);  

        // Writing the "ls" command arg in the pipe.
        const char* arg = ".";
        write(pipefd[1], arg, strlen(arg));
        close(pipefd[1]);

        // Waiting for the son to terminate.
        wait(NULL);

        exit(EXIT_SUCCESS);

    }

    return 0;

}




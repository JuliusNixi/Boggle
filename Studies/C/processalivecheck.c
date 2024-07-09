#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        // Son's code.
        for (int i = 0; i < 5; i++) {
            printf("Son\n");
            fflush(stdout);
            sleep(1);
        }
        printf("Son. Exiting.\n");
        exit(0);
    } else {
        // Father's code.

        int status;
        int child_alive = 1;

        while (child_alive) {

            sleep(2);
            
            // Checks if the son process is alive.
            // From man waitpid:
            /*
            [...]
            WNOHANG
            return immediately if no child has exited.
            [...]
            */
            pid_t result = waitpid(pid, &status, WNOHANG);
            if (result == 0) {
                printf("Father. The son is alive.\n");
            } else if (result == pid) {
                printf("Father. The son is now DEAD.\n");
                child_alive = 0;
            }else{
                perror("waitpid");
                exit(1);
            }
            fflush(stdout);

        }

        printf("Father. Exiting.\n");

    }

    return 0;

}
/*


    THESE ARE EXPERIMENT ON SIGNALS TO BETTER UNDERSTAND WHAT HAPPENS WHEN
    MULTIPLE SIGNALS ARE CHAINED AND DO NOT CAUSE TROUBLE IN THE PROJECT,
    I WILL USE SIGINT AND SIGALRM.

    I EXPECT THE BEHAVIOR EXPLAINED HERE:
    https://stackoverflow.com/questions/18442102/what-happens-when-a-signal-is-received-while-already-in-a-signal-handler

    TL;DR: WE HAVE A BITMAP OF PENDING SIGNALS, BUT THEY ARE NOT ENQUEUED.

    SIGALRM 1 START

    (I SEND 3 CTRL + C)
    SIGINT 1 SENT -> SIGALRM 1 INTERRUPTED
    SIGINT 1 IN RUN
    SIGINT 2 SENT -> PENDING SIGINT 2 SETTED, SIGINT 1 CONTINUE TO RUN
    SIGINT 3 SENT -> PENDING SIGINT 3 SETTED, OVERWRITTEN SIGINT 2 (LOST!),
                     SIGINT 1 CONTINUE TO RUN

    SIGINT SET SIGALRM 2
    SIGALRM 2 RECEIVED -> SETTED SIGALRM 2 TO PENDING, SIGINT 1 CONTINUE TO RUN
    SIGINT SET SIGALRM 3
    SIGALRM 3 RECEIVED -> PENDING SIGALRM 3 SETTED, OVERWRITTEN SIGALRM 2 (LOST!),
                       SIGINT 1 CONTINUE TO RUN
    END SIGINT 1

    SIGINT 3 PENDING STARTED
    SIGINT 3 IN RUN
    END SIGINT 3

    SIGALRM 1 BACK IN RUN
    END SIGALRM 1 

    SIGALRM 3 PENDING STARTED
    SIGALRM 3 IN RUN
    END SIGALRM 3

    BACK TO MAIN... THE LAST (WHEN RECEIVED THE FIRST SIGALRM) SLEEP FAIL!



   ! At the end, I chose a different implementation with a dedicated thread and the sigwait() !
   ! SEE multithreadedsignals.c !

*/


#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

// volatile sig_atomic_t x;
int x; // Seems to work also with int, but is not safe, reccomended the above one
// to mantain signal-async-safety.

void timerHandler(int signum) {

    char strc[] = "START SIGALRM\n";
    write(STDOUT_FILENO, strc, strlen(strc));

    char strc2[] = "SIGNAL SIGALRM CODE: %d.\n";
    sprintf(strc2, strc2, signum);
    write(STDOUT_FILENO, strc2, strlen(strc2));

    while (x < 30){
        sleep(1);
        x++;
        char str[] = "TEST X SIGALRM: %d.\n";
        sprintf(str, str, x);
        write(STDOUT_FILENO, str, strlen(str));
        if (x == 30) return;
    }

    while (x >= 30 && x <= 40){
        sleep(1);
        x++;
        char str[] = "TEST 2 X SIGALRM: %d.\n";
        sprintf(str, str, x);
        write(STDOUT_FILENO, str, strlen(str));
    }
    
    char strc3[] = "END SIGALRM\n";
    write(STDERR_FILENO, strc3, strlen(strc3));

    return;

}


void sigintHandler(int signum) {

    char strc[] = "START SIGINT\n";
    write(STDOUT_FILENO, strc, strlen(strc));

    char strc2[] = "SIGNAL SIGINT CODE: %d.\n";
    sprintf(strc2, strc2, signum);
    write(STDOUT_FILENO, strc2, strlen(strc2));

    while (x < 10){
        sleep(1);
        x++;
        char str[] = "TEST X SIGINT: %d.\n";
        sprintf(str, str, x);
        write(STDOUT_FILENO, str, strlen(str));
    }

    if (x < 20) alarm(2);
    while (x < 20) {
        sleep(1);
        x++;
        char str[] = "TEST X 2 SIGINT: %d.\n";
        sprintf(str, str, x);
        write(STDOUT_FILENO, str, strlen(str));
        if (x == 11) alarm(2);
    }
    char strc3[] = "END SIGINT\n";
    write(STDERR_FILENO, strc3, strlen(strc3));

    return;

}


int main(void) {

    x = 0;
    struct sigaction sigint; // SIGINT signal handler.
    struct sigaction sigactiontimer; // SIGALRM signal handler.

    sigint.sa_handler = sigintHandler;
    int retvalue = sigaction(SIGINT, &sigint, NULL);
    if (retvalue == -1) {
        // Error
        printf("Error in setting SIGINT signal handler.\n");
        exit(EXIT_FAILURE);
    }
    printf("SIGINT signal handler registered correctly.\n");
    printf("SIGINT main code: %d.\n", SIGINT); 

    // Registering SIGALRM signal handler.
    sigactiontimer.sa_handler = timerHandler;
    retvalue = sigaction(SIGALRM, &sigactiontimer, NULL);
    if (retvalue == -1) {
        // Error
        printf("Error in setting SIGALRM signal handler.\n");
    }
    printf("SIGALRM signal handler registered correctly.\n");
    printf("SIGALRM main code: %d.\n", SIGALRM); 

    while (1){
        printf("BACK/START TO MAIN 1.\n");
        alarm(3);
        // SLEEP 1 INTERRUPTED FROM SIGNAL SIGALRM, AT THE RETURN IT WILL FAIL! 
        sleep(2000);
        /////////////////////////////
        printf("BACK TO MAIN 2.\n");
        sleep(2000);
    }
    

    return 0;

}


/*

    OUTPUT
              
SIGINT signal handler registered correctly.
SIGINT main code: 2.
SIGALRM signal handler registered correctly.
SIGALRM main code: 14.
BACK TO MAIN 1.
START SIGALRM
SIGNAL SIGALRM CODE: 14.
TEST X SIGALRM: 1.
TEST X SIGALRM: 2.
^CSTART SIGINT
SIGNAL SIGINT CODE: 2.
^CTEST X SIGINT: 3.
TEST X SIGINT: 4.
^CTEST X SIGINT: 5.
TEST X SIGINT: 6.
TEST X SIGINT: 7.
TEST X SIGINT: 8.
TEST X SIGINT: 9.
TEST X SIGINT: 10.
TEST X 2 SIGINT: 11.
TEST X 2 SIGINT: 12.
TEST X 2 SIGINT: 13.
TEST X 2 SIGINT: 14.
TEST X 2 SIGINT: 15.
TEST X 2 SIGINT: 16.
TEST X 2 SIGINT: 17.
TEST X 2 SIGINT: 18.
TEST X 2 SIGINT: 19.
TEST X 2 SIGINT: 20.
END SIGINT
START SIGINT
SIGNAL SIGINT CODE: 2.
END SIGINT
TEST X SIGALRM: 21.
TEST X SIGALRM: 22.
TEST X SIGALRM: 23.
TEST X SIGALRM: 24.
TEST X SIGALRM: 25.
TEST X SIGALRM: 26.
TEST X SIGALRM: 27.
TEST X SIGALRM: 28.
TEST X SIGALRM: 29.
TEST X SIGALRM: 30.
START SIGALRM
SIGNAL SIGALRM CODE: 14.
END SIGALRM
TEST 2 X SIGALRM: 31.
TEST 2 X SIGALRM: 32.
TEST 2 X SIGALRM: 33.
TEST 2 X SIGALRM: 34.
TEST 2 X SIGALRM: 35.
TEST 2 X SIGALRM: 36.
TEST 2 X SIGALRM: 37.
TEST 2 X SIGALRM: 38.
TEST 2 X SIGALRM: 39.
TEST 2 X SIGALRM: 40.
TEST 2 X SIGALRM: 41.
BACK TO MAIN 2.


*/
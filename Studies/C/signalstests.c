/*


    These tests on signals are done to better understand what happens when
    multiple signals are chained and do not cause trouble in the project.
    I will use SIGINT and SIGALRM.

    I expect the behavior explained here:
    https://stackoverflow.com/questions/18442102/what-happens-when-a-signal-is-received-while-already-in-a-signal-handler

    TL;DR: We have a bitmap of pending signals, but they are not enqueued, so if signal X
    is triggered, during its management another X signal comes, nothing happens, the first
    continue to run and the second makes its own bit change in the pending signal mask. 
    If yet another X signal arrives after that, the process repeats, but the previously
    saved bit to one is merely overwritten. So when the handling of the first signal ends,
    another one will be called on the same signal and the corresponding bit will be set to 0.
    At the termination of this last handling also we will return to where we were when
    interrupted. So although 3 identical signals have been received, only 2 are "really handled"
    and the third is "lost." For each signal we have a bit in the bitmap of the pending signals.

    SIGALRM 1 START

    (I SEND 3 CTRL + C)
    SIGINT 1 SENT -> SIGALRM 1 INTERRUPTED
    SIGINT 1 IN RUN
    SIGINT 2 SENT -> PENDING SIGINT 2 SETTED, SIGINT 1 CONTINUE TO RUN
    SIGINT 3 SENT -> PENDING SIGINT 3 SETTED, OVERWRITTEN SIGINT 2 (LOST!),
                     SIGINT 1 CONTINUE TO RUN

    SIGINT SET SIGALRM 2
    SIGALRM 2 RECEIVED -> SETTED SIGALRM 2 TO PENDING (BECAUSE ALREADY PREVIOUS IN HANDLING),
                          SIGINT 1 CONTINUE TO RUN
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


     IMPORTANT:
     At the end, I chose a different implementation for the project
     with a dedicated thread and the sigwait(). SEE "./multithreadedsignalstests.c"!

*/


#include <stdio.h>
#include <unistd.h>
#include <signal.h>

// volatile sig_atomic_t x;
int x; 
// Seems to work also with int, but is not safe, reccomended the above one
// to mantain signal-async-safety.

// SIGALRM handler.
void timerHandler(int signum) {

    printf("START SIGALRM\n");

    while (x <= 30){
        sleep(1);
        printf("SIGALRM: %d.\n", x++);
        if (x == 30){
            printf("END SIGALRM IN ADVANCE.\n");
            return;
        }
    }

    while (x >= 30 && x <= 40){
        sleep(1);
        printf("SIGALRM 2: %d.\n", x++);
    }
   
    printf("END SIGALRM.\n");

    return;

}

// SIGINT handler.
void sigintHandler(int signum) {

    printf("START SIGINT\n");

    while (x < 10){
        sleep(1);
        printf("SIGINT: %d.\n", x++);
    }

    if (x < 20) alarm(1);
    while (x < 20) {
        sleep(1);
        printf("SIGINT 2: %d.\n", x++);
        if (x == 15) alarm(1);
    }

    printf("END SIGINT.\n");

    return;

}


int main(void) {

    x = 0;

    struct sigaction sigint;
    struct sigaction sigactiontimer;

    // Registering SIGINT signal handler.
    sigint.sa_handler = sigintHandler;
    sigaction(SIGINT, &sigint, NULL);
    // Registering SIGALRM signal handler.
    sigactiontimer.sa_handler = timerHandler;
    sigaction(SIGALRM, &sigactiontimer, NULL);

    while (1){
        printf("BACK/START TO MAIN 1.\n");
        alarm(1);
        // SLEEP 1 INTERRUPTED FROM SIGNAL SIGALRM, AT THE RETURN IT WILL FAIL (STOP SLEEPING)! 
        sleep(120);
        /////////////////////////////
        printf("BACK TO MAIN 2.\n");
        sleep(120);
    }
    

    return 0;

}


/*


    OUTPUT

BACK/START TO MAIN 1.
START SIGALRM
SIGALRM: 0.
^CSTART SIGINT
^CSIGINT: 1.
^CSIGINT: 2.
SIGINT: 3.
SIGINT: 4.
SIGINT: 5.
SIGINT: 6.
SIGINT: 7.
SIGINT: 8.
SIGINT: 9.
SIGINT 2: 10.
SIGINT 2: 11.
SIGINT 2: 12.
SIGINT 2: 13.
SIGINT 2: 14.
SIGINT 2: 15.
SIGINT 2: 16.
SIGINT 2: 17.
SIGINT 2: 18.
SIGINT 2: 19.
END SIGINT.
START SIGINT
END SIGINT.
SIGALRM: 20.
SIGALRM: 21.
SIGALRM: 22.
SIGALRM: 23.
SIGALRM: 24.
SIGALRM: 25.
SIGALRM: 26.
SIGALRM: 27.
SIGALRM: 28.
SIGALRM: 29.
END SIGALRM IN ADVANCE.
START SIGALRM
SIGALRM: 30.
SIGALRM 2: 31.
SIGALRM 2: 32.
SIGALRM 2: 33.
SIGALRM 2: 34.
SIGALRM 2: 35.
SIGALRM 2: 36.
SIGALRM 2: 37.
SIGALRM 2: 38.
SIGALRM 2: 39.
SIGALRM 2: 40.
END SIGALRM.
BACK TO MAIN 2.


*/


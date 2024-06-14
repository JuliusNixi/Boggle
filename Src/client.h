// Shared/Common CLIENT & SERVER cross files vars and libs.
#include "common.h"

// Common client only cross files used vars and libs.
int client_fd;  // Client/Server file descriptor.
#define CHECK_RESPONSES_SECONDS 3 // Time in seconds. Each time a signal SIGALRM will be triggered and the responses received from the server, printed.
pthread_t responses_thread; // It will handle the responses received from the server asynchronously.

// Functions signature client used. Implementation and infos in the client main file.
void* responsesHandler(void*);
void printResponses(void);
void inputHandler(void); 
void checkResponses(int); 
void* cleanerSTDIN(void*);

// Defined, commented and implemented all in common.h and common.c
// int parseIP(char*, struct sockaddr_in*); -> common.h
// void toLowerOrUpperString(char*, char); -> common.h
// struct Message* receiveMessage(int); -> common.h
// void sendMessage(int, char, char*); -> common.h
// void destroyMessage(struct Message**); -> common.h
// void mLock(pthread_mutex_t*); -> common.h
// void mULock(pthread_mutex_t*); -> common.h
// void handleError(int, int, int, int, const char*, ...); -> common.h
// void printff(va_list, const char*, ...); -> common.h
// void makeKey(void); -> common.h
// void threadSetup(void); -> common.h

// Present both in client and server, but with DIFFERENT IMPLEMENTATION.
// void* signalsThread(void*); -> common.h
// void atExit(void) -> common.h
// void threadDestructor(void*); -> common.h
////////////////////////////////////////////////////////////////////////



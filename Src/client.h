// Shared/Common CLIENT & SERVER cross files vars and libs.
#include "common.h"

// Common client only cross files used vars and libs.
int client_fd;  // Client/Server file descriptor.

// Functions signature client used. Implementation and infos in the client main file.
void* responseHandler(void*);

void printResponse(void); // Used to print a server's response. 

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



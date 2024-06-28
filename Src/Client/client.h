// Shared/Common CLIENT & SERVER cross files vars and libs.
#include "common.h"

// Common client only cross files used vars and libs.
int client_fd;  // Client/Server file descriptor.
pthread_t responses_thread; // This thread will handle the responses received from the server asynchronously.

// Functions signatures only client used. Implementation and infos in the client.c file.
void destroyStringList(void);
void setUnblockingGetChar(void);
void setBlockingGetChar(void);
char clearInput(void);
char sanitizeCheckInput(void);
void insertStringInList(void);
void checkResponses(void);
void processInput(void);
void inputHandler(void);
void* responsesHandler(void*);
void printResponses(void);

// Defined, commented and implemented all in common.h and common.c.
// int parseIP(char*, struct sockaddr_in*); -> common.h
// void toLowerOrUpperString(char*, char); -> common.h
// struct Message* receiveMessage(int); -> common.h
// void* sendMessage(int, char, char*); -> common.h
// void destroyMessage(struct Message**); -> common.h
// void mLock(pthread_mutex_t*); -> common.h
// void mULock(pthread_mutex_t*); -> common.h
// void handleError(char, char, char, char, const char*, ...); -> common.h
// void printff(va_list, char, const char*, ...); -> common.h
// void makeKey(void); -> common.h
// void threadSetup(void); -> common.h
// char* bannerCreator(uli, uli, char*, char, char); -> common.h

// Present both in client and server, but with DIFFERENT IMPLEMENTATION.
// void* signalsThread(void*); -> common.h
// void atExit(void) -> common.h
// void threadDestructor(void*); -> common.h
////////////////////////////////////////////////////////////////////////



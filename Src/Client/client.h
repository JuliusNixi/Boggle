// Shared/Common CLIENT & SERVER cross files vars and libs.
#include "../Common/common.h"

// Common client only cross files used vars and libs.
int client_fd; // Client/Server socket file descriptor.

pthread_t responsesthread; // This thread will handle the responses received from the server asynchronously.

// Functions signatures client used in client.c and boggle_client.c.
// Implementation and infos in the client.c file.
void setUnblockingGetChar(void);
void setBlockingGetChar(void);
char clearInput(void);
void inputHandler(void);
void* responsesHandler(void*);

// Defined, commented and implemented all in common.h and common.c.
// int parseIP(char*, struct sockaddr_in*); -> common.h
// void toLowerOrUpperString(char*, char); -> common.h
// struct Message* receiveMessage(int, char*); -> common.h
// char sendMessage(int, char, char*); -> common.h
// void destroyMessage(struct Message**); -> common.h
// char* bannerCreator(uli, uli, char*, char, char); -> common.h

// Present both in client and server, but with DIFFERENT IMPLEMENTATION.
// void* signalsThread(void*); -> common.h
////////////////////////////////////////////////////////////////////////



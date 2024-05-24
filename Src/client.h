// Shared/Common CLIENT & SERVER files vars and libs.
#include "common.h"

// Common client only files used vars and libs.
int client_fd;  // Client/Server file descriptor.

pthread_mutex_t mprint; // Mutex used by the main thread and the responses handler thread to sync printing.
int aprint; // Support var used with the mutex above.

#define PROMPT_STR "[PROMPT PAROLIERE]--> " // Prompt string.

// Functions signature client used. Implementation and infos in the client main file.
void* responseHandler(void*);
// Present both in client and server, but with DIFFERENT IMPLEMENTATION.
// void clearExit(void); -> common.h
// void* signalsThread(void* args); -> common.h
////////////////////////////////////////////////////////////////////////
// struct Message* receiveMessage(int); -> common.h
// void sendMessage(int, char, char*); -> common.h
// void destroyMessage(struct Message**); -> common.h
// int parseIP(char*, struct sockaddr_in*); -> common.h
// void toLowerOrUpperString(char*, char); -> common.h

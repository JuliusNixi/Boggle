#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(void){

    typedef unsigned long int uli; // Shortcut used sometimes.


    char* s = "unregistered,0";
    char* backup = (char*) malloc(sizeof(char) * (strlen(s) + 1));
    strcpy(backup, s);
    // Terminating string.
    backup[strlen(s)] = '\0';

//    unregistered\0'0'\0

    // Getting name.
    char* tmp = strtok(backup, ",");
    uli namelen = strlen(tmp) + 1;
    char* name = (char*) malloc(sizeof(char) * namelen);
    strcpy(name, tmp);
    // Terminating string.
    name[strlen(tmp)] = '\0';

    // Getting points.
    tmp = strtok(NULL, ",");
    uli pointslen = strlen(tmp) + 1;
    char* points = (char*) malloc(sizeof(char) * pointslen);

    printf("OK %s\n",tmp);

    return 0;
}
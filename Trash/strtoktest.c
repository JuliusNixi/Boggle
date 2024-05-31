#include <stdio.h>
#include <string.h>

int main(void){

    char str[] = "hello,mum";

    char* s = strtok(str, ",");
    printf("X 1: %s\n", s);
    s = strtok(NULL, ",");
    printf("X 2: %s\n", s);
    s = strtok(NULL, ",");
    printf("X 3: %s\n", s);
    printf("STR INTIAL: %s\n", str);

    return 0;

}



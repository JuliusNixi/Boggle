#include "../Src/support_functions.h"

extern char matrixstring[];

#include <stdio.h>

int main(void) {

    initMatrix();
    loadDictionary();
    printf("loadMatrixFromFile...\n");    
    loadMatrixFromFile("../Data/matrici2.txt");
    loadMatrixFromFile(NULL);
    serializeMatrixStr();
    printf("%s\n", matrixstring);

}

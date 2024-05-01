#include "data_structures.h"
#include "support_functions.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>

void generateRandomMatrix(void) {

    char alphabet[] = "ABCDEFGHILMNOPQRSTUVZ";
    srand(time(NULL));
    int randint = rand() % strlen(alphabet);
    for (int i = 0; i < NROWS; i++)
        for (int j = 0; j < NCOL; j++)
            matrix[i][j] = alphabet[randint];

}


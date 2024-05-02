#include "data_structures.h"
#include "support_functions.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>

// Tested OK
void generateRandomMatrix(void) {

    char alphabet[] = "ABCDEFGHILMNOPQRSTUVZ";
    srand(time(NULL));
    int randint;
    for (int i = 0; i < NROWS; i++)
        for (int j = 0; j < NCOL; j++) {
            randint = rand() % strlen(alphabet);
            matrix[i][j] = alphabet[randint];
        }

}

// Tested OK
void getMatrixNextIndexes(int* matrixnextindexes) {

    static int i;
    static int j;
    if (matrixnextindexes == NULL) {
        // Initialization or reset.
        i = 0;
        j = 0;
        return;
    }
    for (; i < NROWS; ) {
        for (; j < NCOL; ) {
            matrixnextindexes[0] = i;
            matrixnextindexes[1] = j++;
            if (j != NCOL)
                return;
            j = 0;
            break;
        }
        i++;
        return;
    }

    // Reset.
    matrixnextindexes[0] = -1;
    matrixnextindexes[1] = -1;
    getMatrixNextIndexes(NULL);

}

// Tested OK
void serializeMatrixStr(void) {

    // Initializing the string.
    int counter = 0;
    while (counter < matrixstrlength)
        matrixstring[counter++] = 'X';

    // Inserting \n.
    int r = 0;
    counter = 0;
    while (counter < matrixstrlength) {
        if (r != 0 && r % ((NCOL * 2) + (NCOL - 1)) == 0) {
            matrixstring[counter] = '\n';
            r = 0;
            counter++;
            continue;
        }
        counter++;
        r++;
    }

    // Inserting letters.
    counter = 0;
    int letters = 0;
    getMatrixNextIndexes(NULL);
    int matrixnextindexes[2];
    while (counter < matrixstrlength) {
        if (matrixstring[counter] == '\n') {
            letters = 0;
            counter++;
            continue;
        }
        if (letters % 3 == 0 && counter + 1 != matrixstrlength) {
            getMatrixNextIndexes(matrixnextindexes);
            matrixstring[counter] = matrix[matrixnextindexes[0]][matrixnextindexes[1]];
            if (matrixstring[counter] == 'Q') {
                matrixstring[++counter] = 'u';
                ++letters;
            }
        }
        letters++;
        counter++;
    }

    // Inserting spaces.
    counter = 0;
    while (counter < matrixstrlength) {
        if (matrixstring[counter] == 'X' && counter + 1 != matrixstrlength)
            matrixstring[counter] = ' ';
        counter++;
    }

    // Insert string terminator.
    matrixstring[matrixstrlength - 1] = '\0';

}

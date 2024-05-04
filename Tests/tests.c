#include "../Src/support_functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char** matrix;
extern const int NROWS;
extern const int NCOL;
extern char alphabet[];
extern char matrixstring[]; 

int main(void) {

    int tests = 5;
    srand(42);

    // Testing generateRandomMatrix.
    printf("Testing generateRandomMatrix...\n");
    for (int x = 0; x < tests; x++) {
        generateRandomMatrix();
        for (int i = 0; i < NROWS; i++) {
            for (int j = 0; j < NCOL; j++)
                printf("%c ", matrix[i][j]);
            printf("\n");
        }
        printf("\n");
    }

    // Testing getMatrixNextIndexes.
    printf("Testing getMatrixNextIndexes...\n");
    int matrixnextindexes[2];
    for (int i = 0; i < tests; i++) {
        getMatrixNextIndexes(matrixnextindexes);
        printf("i: %d j: %d \n", matrixnextindexes[0], matrixnextindexes[1]);
    }
    printf("\n");

    // Testing serializeMatrixStr.
    printf("Testing serializeMatrixStr...\n");
    for (int x = 0; x < tests; x++) {
        generateRandomMatrix();
        serializeMatrixStr();
        printf("%s", matrixstring);
        printf("--------------ABOVE THE SERIALIZED, BELOW THE NORMAL--------------\n");
        for (int i = 0; i < NROWS; i++) {
            for (int j = 0; j < NCOL; j++)
                printf("%c ", matrix[i][j]);
            printf("\n");
        }    
        printf("\n");  
    }

    // Testing validateMatrix.
    printf("Testing validateMatrix...\n");
    for (int x = 0; x < tests; x++) {
        // Changing random character in the matrix with some not allowed.
        for (int i = 0; i < NROWS; i++)
            for (int j = 0; j < NCOL; j++) {
                // Including in the new alphabet some letters not contained in the legit one.
                char newalpha[] = "JABCDEFGHILMNOPQRSTUVZ"; // JKWXY
                int r = rand() % strlen(newalpha);
                char c = newalpha[r];
                matrix[i][j] = c;
            }
        // Remember not to use the serializeMatrixStr to print
        // because it validate the matrix itself.
        for (int i = 0; i < NROWS; i++) {
            for (int j = 0; j < NCOL; j++)
                printf("%c ", matrix[i][j]);
            printf("\n");
        }
        // If the matrix is not valid should print something, nothing otherwise.
        printf("--------------\n");
        validateMatrix();
        printf("--------------\n");
    }

    // Testing loadMatrixFromFile.
    printf("Testing loadMatrixFromFile...\n");    
    for (int i = 0; i < tests; i++) {
        loadMatrixFromFile("../Data/matrici.txt");
        serializeMatrixStr();
        printf("%s\n", matrixstring);
    }

    return 0;
}

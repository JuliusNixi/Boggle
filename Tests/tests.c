#include "../Src/support_functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char matrixstring[];

#define NROWS 4
#define NCOL 4
char matrix[NROWS][NCOL];

int main(void) {
    
    int tests = 5;

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
        if (i == 0) loadMatrixFromFile("../Data/matrici.txt");
        serializeMatrixStr();
        printf("%s\n", matrixstring);
    }

/*
    // se cambiano dimensioni matrici modificare qui
    int m[][4] = {
                {1,2,-1,-1},        {-1,3,4,-1},        {-1,-1,5,6},        {-1,-1,-1,7}

        };
    for (int i = 0; i < NROWS; i++){
        for (int j = 0; j < NCOL; j++) {
            // copio solo i numeri (forzando manualmente) non voglio testare con parole ora
            matrixint[i][j] = m[i][j];
            printf("%d ", m[i][j]);
        }
        printf("\n");
    }

    printf("\n");
    */
   
/*
    for (int i = 0; i < NROWS; i++){
        for (int j = 0; j < NCOL; j++)
            printf("%d ", validatePattern(i, j));
        printf("\n");
    }

    printf("\n");
    loadMatrixFromFile("../Data/matrici2.txt");
    serializeMatrixStr();
    loadDictionary();
    printf("New matrix:\n%s",matrixstring);
    char s1[] = "BACKPACK";
    char s2[] = "ABCDEFG";
    char s3[] = "NOT IN DICT";
    printf("SEARCHING %s EXPECTED 0 -> %d.\n", s1, searchInMatrix(s1));
    printf("SEARCHING %s EXPECTED 1 -> %d.\n", s2, searchInMatrix(s2));
    printf("SEARCHING %s EXPECTED 0 -> %d.\n", s3, searchInMatrix(s3));
    for (int i = 0; i < NROWS; i++){
        for (int j = 0; j < NCOL; j++) {
            // copio solo i numeri (forzando manualmente) non voglio testare con parole ora
            printf("%d ", matrixint[i][j]);
        }
        printf("\n");
    }



    printf("\n\n\n");
    loadMatrixFromFile(NULL);
    serializeMatrixStr();
    printf("New matrix:\n%s",matrixstring);
    char s4[] = "CASA";
    printf("SEARCHING %s EXPECTED 1 -> %d.\n", s4, searchInMatrix(s4));







    extern int cerca(char*);
    printf("\n\n\n");
    cerca("CASA");
*/
    return 0;
    
}

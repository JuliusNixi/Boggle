#include "../../Src/Server/server.h"

// Remember to compile with also "../../Src/Common/common.c" and "../../Src/Server/server.c".

// This file contains some basic tests on some functions.

// Printing the game matrix without using serializeMatrixStr() to perform the tests.
void printManualMatrix(void) {
    for (uli i = 0LU; i < NROWS; i++) {
        for (uli j = 0LU; j < NCOL; j++)
            printf("%c ", matrix[i][j]);
        printf("\n");
    }
}

int main(void) {

    // Setting the rand seed.
    srand(RAND_SEED);

    // Number of tests.
    int tests = 5;

    // Testing generateRandomMatrix.
    printf("Testing generateRandomMatrix...\n");
    for (uli i = 0LU; i < tests; i++) {
        generateRandomMatrix();
        printManualMatrix();
    }

    // Testing getMatrixNextIndexes.
    printf("Testing getMatrixNextIndexes...\n");
    int matrixnextindexes[2];
    for (uli i = 0LU; i < tests * 4; i++) {
        getMatrixNextIndexes(matrixnextindexes);
        printf("i: %d j: %d \n", matrixnextindexes[0], matrixnextindexes[1]);
    }

    // Testing validateMatrix.
    printf("Testing validateMatrix...\n");
    char temp = matrix[0][0];
    matrix[0][0] = VOID_CHAR;
    printf("Expecting error...\n");
    validateMatrix();
    matrix[0][0] = temp;
    validateMatrix();
    printf("Expeting pass (nothing)...\n");

    // Testing serializeMatrixStr.
    printf("Testing serializeMatrixStr...\n");
    for (uli i = 0LU; i < tests; i++) {
        printf("---------------------------------\n");
        generateRandomMatrix();
        printf("*********************************\n");
        char* s = serializeMatrixStr();
        printf("%s", s);
        free(s);
        printf("--------------ABOVE THE SERIALIZED, BELOW THE NORMAL (MANUAL)--------------\n");
        printManualMatrix();
        printf("---------------------------------\n");
    }

    // Re-testing validateMatrix.
    printf("Re-testing validateMatrix...\n");
    // Including in the new alphabet some letters not contained in the legit one.
    char trash[] = "-.,()/&!|$£";
    size_t l = strlen(ALPHABET) + strlen(trash);
    char newalpha[l];
    strcat(newalpha, ALPHABET);
    strcat(newalpha, trash);
    toLowerOrUpperString(newalpha, 'U');
    for (uli x = 0LU; x < tests; x++) {
        // Changing random character in the matrix with some not allowed.
        for (uli i = 0LU; i < NROWS; i++)
            for (uli j = 0LU; j < NCOL; j++) {
                // Choosing random char of the new alphabet.
                int r = rand() % strlen(newalpha);
                char c = newalpha[r];
                // Choosing between changing the matrix[i][j] element with a random not allowed char, or not.
                r = rand() % 10;
                if (r <= 3) matrix[i][j] = c;
            }
        // Remember not to use the serializeMatrixStr to print
        // because it validate the matrix itself.
        printManualMatrix();
        // If the matrix is not valid should print something, nothing otherwise.
        printf("--------------\n");
        validateMatrix();
        printf("--------------\n");
    }

    // Testing loadMatrixFromFile.
    printf("Testing loadMatrixFromFile...\n");
    for (uli i = 0LU; i < tests; i++) {
        // Loading all the file the first time.
        if (i == 0) loadMatrixFromFile("../../Data/Matrices/mymatrices.txt");
        // Loading the next matrix (file row).
        else loadMatrixFromFile(NULL);
        char* s = serializeMatrixStr();
        printf("%s\n", s);
        free(s);
    }


    /*
        TO TEST
        void loadDictionary(char*);
        int searchWordInMatrix(int, int, char*);
        void validateDictionary(void);
        int validateWord(char*);
    */

  
    return 0;
    
}



#include "../Src/server.h"

extern unsigned int duration;


// Printing game matrix without serializeMatrixStr
void printManualMatrix(void) {
    for (int i = 0; i < NROWS; i++) {
            for (int j = 0; j < NCOL; j++)
                printf("%c ", matrix[i][j]);
            printf("\n");
    }
}

int main(void) {

    srand(42);

    int tests = 5;

    // Testing initMatrix.
    printf("Testing initMatrix...\n");
    initMatrix();
    printManualMatrix();

    // Testing generateRandomMatrix.
    printf("Testing generateRandomMatrix...\n");
    for (int x = 0; x < tests; x++) {
        generateRandomMatrix();
        printManualMatrix();
    }

    // Testing getMatrixNextIndexes.
    printf("Testing getMatrixNextIndexes...\n");
    int matrixnextindexes[2];
    for (int i = 0; i < tests; i++) {
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
    for (int x = 0; x < tests; x++) {
        generateRandomMatrix();
        char* s = serializeMatrixStr();
        printf("%s", s);
        free(s);
        printf("--------------ABOVE THE SERIALIZED, BELOW THE NORMAL--------------\n");
        printManualMatrix();
    }

    // Re-testing validateMatrix.
    printf("Re-testing validateMatrix...\n");
    for (int x = 0; x < tests; x++) {
        // Changing random character in the matrix with some not allowed.
        for (int i = 0; i < NROWS; i++)
            for (int j = 0; j < NCOL; j++) {
                // Including in the new alphabet some letters not contained in the legit one.
                char trash[] = "-.,()/&!|$£";
                size_t l = strlen(ALPHABET) + strlen(trash);
                char newalpha[l];
                strcat(newalpha, ALPHABET);
                strcat(newalpha, trash);
                int r = rand() % strlen(newalpha);
                char c = newalpha[r];
                r = rand() % 10;
                // Choosing between changing the matrix[i][j] element with a random not allowed char, or not.
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
    for (int i = 0; i < tests; i++) {
        if (i == 0) loadMatrixFromFile("../Data/matriciprof.txt");
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

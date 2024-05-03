#include "data_structures.h"
#include "support_functions.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

// Tested OK
void generateRandomMatrix(void) {

    // This function generate a random letter matrix of size as written in NCOL and NROWS.
    // The matrix is global, so it can be modified by side effect.

    // Initializing the random integer generator with the seed of the current time.
    srand(time(NULL));
    int randint;
    for (int i = 0; i < NROWS; i++)
        for (int j = 0; j < NCOL; j++) {
            // Choosing a random letter from alphabet and filling the matrix.
            randint = rand() % strlen(alphabet);
            matrix[i][j] = alphabet[randint];
        }

}

// Tested OK
void getMatrixNextIndexes(int* matrixnextindexes) {

    // This function is an iterator, useful to read/write matrix.
    // It takes as input an integer array of 2 positions.
    // It fills array[0] with the matrix current row index.
    // It fills array[1] with the matrix current column index.

    // Static, persitent between function calls.
    static int i;
    static int j;
    if (matrixnextindexes == NULL) {
        // Initialization or reset of the iterator is made by passing NULL as arg.
        i = 0;
        j = 0;
        return;
    }
    // Generating next indexes.
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
    // If we arrive here it means that we are trying to do an out of bound matrix access.
    // I report it by returning -1. Then i reset the iterator with a recursive call.
    matrixnextindexes[0] = -1;
    matrixnextindexes[1] = -1;
    getMatrixNextIndexes(NULL);

}

void validateMatrix(void) {

    // This function check if the matrix stored in the global var is legit.
    // If is found in the matrix at least 1 charcater not present in the alphabet,
    // the matrix is not valid, and a critical error is throw.
    // If the matrix is valid, nothing happen.

    // Initializing iterator.
    getMatrixNextIndexes(NULL);
    int matrixnextindexes[2];
    for (int i = 0; i < NROWS; i++) {
        for (int j = 0; j < NCOL; j++){
            getMatrixNextIndexes(matrixnextindexes);
            char c = matrix[matrixnextindexes[0]][matrixnextindexes[1]];
            int found = 0;
            // Searching char of matrix in the alphabet.
            for (int x = 0; x < strlen(alphabet); x++)
                if (c == alphabet[x]) {
                    found = 1;
                    break;
                }
            // Character not found, error.
            if (found == 0) {
                // Error
            }
        }
    }

}

// Tested OK
void serializeMatrixStr(void) {

    // This functions fills a global string that will rapresent the matrix.

    // Checking the matrix validation.
    validateMatrix();

    // Initializing the string with a special character X, useful for testing and debugging.
    int counter = 0;
    while (counter < matrixstrlength)
        matrixstring[counter++] = 'X';

    // Inserting \n at the end of each row.
    int r = 0;
    counter = 0;
    while (counter < matrixstrlength) {
        // I use the module to understand when end of row in string is happened.
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
    // Iterator initialized.
    getMatrixNextIndexes(NULL);
    int matrixnextindexes[2];
    while (counter < matrixstrlength) {
        // End of line reached.
        if (matrixstring[counter] == '\n') {
            letters = 0;
            counter++;
            continue;
        }
        // Inserting next matrix letter in the string.
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

void loadMatrixFromFile(char* path) {

    // This function generate a matrix of size as written in NCOL and NROWS by reading a file.
    // The matrix is global, so it can be modified by side effect.
    // The file path is also store globally.

   // Stat to get file information. retvalue to check syscalls returns.
   struct stat s;
   int retvalue;

   // Performing stat on file.
   retvalue = stat(path, &s);
   if (retvalue == -1) {
    // Error
   }
   // Check if the file is regular.
   if(!S_ISREG(s.st_mode)){
        // Error
        printf("%s is not a regular file.\n", path);
        exit(EXIT_FAILURE);
    }

    // To store file content.
    char file[s.st_size];

    // Opening the file in readonly mode.
    int fd = open(path, O_RDONLY, NULL);
    if (fd == -1) {
        // Error
    }

    // Reading the file content using a buffer of BUFFER_SIZE length.
    char buffer[BUFFER_SIZE];
    int counter = 0;
    while (1) {
        retvalue = read(fd, buffer, BUFFER_SIZE);
        if (retvalue == -1) {
            // Error
        }
        // Exit, end of file reached.
        if (retvalue == 0) break;

        // Copying the buffer in the main array.
        for (int i = 0; i < retvalue; i++)
            file[(counter++)] = buffer[i];
    }

    // Applying the read matrix from file to the program matrix var.
    // Generating a random matrix to handle the case of empty matrix.
    generateRandomMatrix();
    // Initializing iterator.
    getMatrixNextIndexes(NULL);
    int matrixnextindexes[2];
    for (int i = 0; i < s.st_size; i++) {
        // Skipping spaces, new lines, and 'u' of 'Qu'.
        if (file[i] == ' ' || file[i] == 'u' || file[i] == '\n') continue;
        // Getting next matrix indexes to write.
        getMatrixNextIndexes(matrixnextindexes);
        matrix[matrixnextindexes[0]][matrixnextindexes[1]] = file[i];
    }

    // Validating the new matrix.
    validateMatrix();

}

int searchWord(char* word) {

    /*int matrixcheck[NROWS][NCOL];

    // Initializing iterator.
    getMatrixNextIndexes(NULL);
    int matrixnextindexes[2];
    while (matrixnextindexes[0] != -1) {

    }
*/

    return 0;

}


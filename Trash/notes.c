/*
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include "macro.h"
#include <stdlib.h>
#include <stdio.h>


# Client ip e porta
INET_ADDRSTRLEN size of buffer
char buf [INET_ADDRSTRLEN] = {0}; 
         #include <arpa/inet.h>
	inet_ntop(AF_INET, &(new->client_addr.sin_addr), buf, new->client_address_len);
    printf("%d %s\n", new->client_addr.sin_port, buf);



 binarySearch(arr, 0, n - 1, x);

    
    // Ricezione del messaggio
    ssize_t n_read = 0;
    char buffer[BUFFER_SIZE];
    n_read =  read(new->socket_client_fd, buffer, BUFFER_SIZE);
    printf("\n%s\n",buffer);
    

*/






/*
// Merges two subarrays of arr[].
// First subarray is arr[l..m].
// Second subarray is arr[m+1..r].
void mergeChar(char arr[], int l, int m, int r)
{
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;

    // Create temp arrays.
    char L[n1], R[n2];

    // Copy data to temp arrays L[] and R[].
    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];

    // Merge the temp arrays back into arr[l..r].
    i = 0;
    j = 0;
    k = l;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        }
        else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    // Copy the remaining elements of L[],
    // if there are any.
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    // Copy the remaining elements of R[],
    // if there are any.
    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }

}

// l is for left index and r is right index of the
// sub-array of arr to be sorted.
void mergeSortCharK(char arr[], int l, int r)
{
    
    if (l < r) {

        int m = l + (r - l) / 2;

        // Sort first and second halves.
        mergeSortCharK(arr, l, m);
        mergeSortCharK(arr, m + 1, r);

        mergeChar(arr, l, m, r);

    }
    
}

// Wrapper for mergeSortK, to fix \0 character.
// Merge sort string characters.
void mergeSortChar(char arr[], int l, int r)  {
    
    int arr_size = r + 1;
    mergeSortCharK(arr, l, r);
  
    int counter = 1;
    for (int i = 0; i < arr_size; i++) {
        if (i == arr_size - 1) break;
        arr[i] = arr[counter++];
    }
    
    arr[arr_size - 1] = '\0';
    
}


//char arr[] = "ciao";
//int arr_size = sizeof(arr) / sizeof(arr[0]);
//mergeSort(arr, 0, arr_size - 1);


// An iterative binary search function on string characters.
int binarySearch(char arr[], int l, int r, char element)
{
    while (l <= r) {
        int m = l + (r - l) / 2;

        // Check if element is present at mid.
        if (arr[m] == element)
            return m;

        // If element greater, ignore left half.
        if (arr[m] < element)
            l = m + 1;

        // If element is smaller, ignore right half.
        else
            r = m - 1;
    }

    // If we reach here, then element was not present.
    return -1;
}
*/

/*

void mergeChar(char*, int, int, int);
void mergeSortCharK(char*, int, int);
void mergeSortChar(char*, int, int);
int binarySearch(char*, int, int, char);

*/











/*

// A simple function that searches a character in a string.
// It returns the char position in the string, otherwise -1.
int searchInString(char* string, char c) {

    for (int i = 0; i < strlen(string); i++)
        if (string[i] == c) return i;
    return -1;

}

// Validate a pattern of word in the game matrix.
int validatePattern(int i, int j) {
   if (matrix == NULL) initMatrix();
   if (i < 0 || i > NROWS - 1 || j < 0 || j > NCOL - 1) return 0;
   int e = matrixint[i][j];
   if (e == -1) return 0;
   else {
     return e + validatePattern(i, j - 1) + validatePattern(i - 1, j);
   }
}

*/

/*

// Search a word in the dictionary file.
// word is the string to search in the dictionary.
// return 1 if find, 0 otherwise.
int validateDictionary(char* word) {

    for (int i = 0; i < words_len; i++) 
        if (strcmp(words[i], word) == 0) return 1;
    return 0;

}

// Search a word submitted by a player/client in the game matrix.
// returns 1 if the word is founded in the dictionary AND in the game matrix.
// if one of both fail the returns will be 0.
int searchInMatrix(char* word) {

    if (validateDictionary(word) == 0) return 0;
    
    for (int i = 0; i < NROWS; i++)
        for (int j = 0; j < NCOL; j++) {
            // TODO
        }

    return 0;

}

char** copyMatrix(void) {

    char** c;
    c = (char**) malloc(sizeof(char*) * NROWS);
    for (int i = 0; i < NCOL; i++)
        c[i] = (char*) malloc(sizeof(char) * NCOL);
    for (int i = 0; i < NROWS; i++) {
        for (int j = 0; j < NCOL; j++)
            c[i][j] = matrix[i][j];
    }
    return c;

}

int check(int i, int j, char* s, char** tmpmatrix) {

   if (tmpmatrix == NULL) // Error ();
   if (s == NULL) return 0;
   if (i < 0 || i > NROWS - 1 || j < 0 || j > NCOL - 1) return 0;
    char c = s[0];
    if (c == '\0'){
        printf("TROVATO %d %d\n",i,j);
        return 1;
    } 
    printf("cerco %c elemento %c i: %d  j: %d -> %d\n",c, tmpmatrix[i][j],i,j,(int)c);
    if (tmpmatrix[i][j] == c) {
        s++;
        tmpmatrix[i][j] = 'X';
        // shortcut
        return check(i - 1, j, s,tmpmatrix) || check(i, j - 1, s,tmpmatrix) || check(i, j + 1, s,tmpmatrix) || check(i + 1, j, s,tmpmatrix);
    }
    return 0;

}

int cerca(char* word) {

    char** c = copyMatrix();
    for (int i = 0; i < NROWS; i++) {
        for (int j = 0; j < NCOL; j++) {
            printf("Matrice char for: %c \n", matrix[i][j]);
            int x = check(i,j, word,c);
            printf("Valore %d \n", x);
            c = copyMatrix();
        }
    }
    return 0;
}

char** copyD(void) {

    char** d;
    d = (char**) malloc(sizeof(char*) * words_len);
    for (int i = 0; i < words_len; i++) {
        d[i] = (char*) malloc(sizeof(char) * strlen(words[i]));
        strcpy(d[i], words[i]);
    }
    return d;

}

*/







/*

// This function search, starting from an element of the current game matrix,
// rapresented by the inputs i and j (respectively row, column indexes)
// all the words present in the global var "char** words".
// It uses another global var "char** words_copy", and modifies this one.
// It updates every word in "char** words_copy" by removing the letter found 
// in the current game matrix from it.
// If all the letters of a word are founded at end, this word in the "char** words_copy"
// will be rapresented as '\0'.
void searchWordInMatrix(int i, int j) {

    // i and j are not valid. Trying to do an access out of bounds to the game matrix.
    // We must stop the recursive call.
    if (i < 0 || i > NROWS - 1 || j < 0 || j > NCOL - 1) return;

    // foundatleastone signals if at least one letter of a word is founded.
    // So if it positive, it means we should continue to check the adjacent
    // elements in the matrix, because it is possible that the word is present.
    // On the other hand, if it is 0, it means we must stop the search.
    int foundatleastone = 0;
    // Iterating on all words loaded with loadDictionary() from dictionary file
    // in the "char** words".
    for (int l = 0; l < words_len; l++) {
 
        // Current char element in the l-word to find.
        char c = words_copy[l][0];
        // Word already founded in the matrix, skipping to the next.
        if (c == '\0') continue;
        // Checking the Qu case in the current game matrix and in the word-l, to handle the situation.
        if (c == 'Q' && words_copy[l][1] == 'U' && matrix[i][j] == 'Q') (words_copy[l])++;
        // Is present the current char element of the l-word in the matrix[i][j]?
        if (matrix[i][j] == c) {
            // Yes, at least one letter found.
            // Incrementing the pointer of the current l-word, in this way we will
            // continue to search for the next word char.
            (words_copy[l])++;
            foundatleastone++;
        } 
        
    }

    // Continuing the search on the adjacent elements.
    if (foundatleastone) {
        searchWordInMatrix(i - 1, j);
        searchWordInMatrix(i, j - 1);
        searchWordInMatrix(i, j + 1);
        searchWordInMatrix(i + 1, j);
    }

}

// The previous function searchWordInMatrix() modifies the "char** words_copy" var.
// At the end words_copy will contain, for each word of the dictionary file
// previous loaded (with loadDictionary()), only the remaining letters of
// the initial word that were not found in the current game matrix starting
// the search from an element of it.
// The words of which all letters have been found will be represented by '\0'.
// The function has two different behaviors, if it is the first time it is called
// (reported with words_copy[0] == NULL) it will just copy all words from "char** words"
// to "char** words_copy". If the function call, on the other hand, should be one after the 
// first, it will reset all words that have not been completely found, i.e.,
// those other than '\0'.
void restoreWords(void) {
    // First behavior.
    if (words_copy[0] == NULL) {
        for (int i = 0; i < words_len; i++)
            words_copy[i] = words[i];
        return;
    }
    // Second behavior.
    for (int i = 0; i < words_len; i++)
        if (words_copy[i][0] != '\0') words_copy[i] = words[i];
}

// This function validate a dictionary file previous loaded with loadDictionary().
// Validate means that "delete" all words loaded from the dictionary that are not
// present in the current game matrix.
// All the words present in the file are stored (with loadDictionary()) in the
// global "char** words" var.
// After this function execution, there will be another "char** words_copy" global
// var that will contain only the words present in the current game matrix (and
// obviously in the file, and so in the "char** words").
// WARNING: It MUST CALLED AFTER the function loadDictionary() to load
// the dictionary before.
void validateDictionary(void) {

    // Dictionary not loaded.
    if (words == 0) {
        // Error
        printf("Error, cannot validate the words if a dictionary has not been previously loaded.\nCall loadDictionary() before and retry.\n");
    }

    // First validation. words_copy must be initialized.
    if (words_copy == 0) {
        words_copy = (char**) malloc(sizeof(char*) * words_len);
        if (words_copy == 0) {
            // Error
            printf("Error, cannot allocate heap memory for the words_copy.\n");
        }
    }

    // Setting the first char* in the array to NULL to signal the restoreWords()
    // function that is the first call of it.
    // The function has a different behavior the first time, compared with subsequent.
    words_copy[0] = NULL;
    restoreWords();

    // Iter on the letters in the current game matrix. 
    // For each of them I invoke an searchWordInMatrix()
    // function that will search for all the words contained in "char** words" (dictionary file),
    // and the restoreWords(), that will reset the words strings modified but
    // not completely founded ini the words_copy var.
    for (int i = 0; i < NROWS; i++)
        for (int j = 0; j < NCOL; j++) {
            searchWordInMatrix(i, j);
            restoreWords();
        }

    // Now words_copy should contain the words NOT present in the current game matrix,
    // instead, the words present will be reported as '\0'.
    // Now we will "invert" the dictionary, because we want the words present in the matrix.
    // In other words, we can say that we will calculate the complementary of the set.
    // At the end of this for, words_copy will contain only the words present in the 
    // current game matrix, instead, the words NOT present will be reported as '\0'.
    for (int i = 0; i < words_len; i++)
        if (words_copy[i][0] == '\0') words_copy[i] = words[i];
        else while ((words_copy[i][0])++ != '\0');

    printf("Dictionary succesfully validated, founded in the current matrix, these words from dict file:\n");
    for (int i = 0; i < words_len; i++)
        if (words_copy[i][0] != '\0')
            printf("%s\n", words_copy[i]);

}

*/




















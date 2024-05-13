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


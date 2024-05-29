/*

#include <sys/socket.h>




 binarySearch(arr, 0, n - 1, x);


    

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





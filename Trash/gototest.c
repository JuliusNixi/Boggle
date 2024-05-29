#include <stdio.h>

void f(int x) {

int y = x - 1;

label:

    printf("%d %d\n", x, y);
    x++;

    goto label;


}

int main(void) {

    f(0);
    return 0;

}


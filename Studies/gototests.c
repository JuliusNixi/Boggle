#include <stdio.h>

// Goto tests.

void f(int x) {

    int y = x - 1;

    // Brackets block are necessary!
    label: {

        printf("%d %d\n", x, y);
        x++;
    }
        goto label;


}

int main(void) {

    f(0);
    return 0;

}


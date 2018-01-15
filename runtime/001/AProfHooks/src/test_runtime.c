#include <stdio.h>

int test_for(int *a) {
    int b = *a;
    /* for loop execution */
    for (*a = 1; *a < 2; (*a)++) {
        printf("value of a: %d\n", *a);
    }

    printf("a is %d \n", *a);
    return 0;
}

int main() {

    int a = 1;
//
//    /* for loop execution */
//    for (a = 1; a < 2; a++) {
//        printf("value of a: %d\n", a);
//    }

    test_for(&a);

    printf("value of a: %d\n", a);
    return 0;
}

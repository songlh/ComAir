#include <stdio.h>

int test_for() {
    int a;

    /* for loop execution */
    for (a = 1; a < 10; a++) {
        printf("value of a: %d\n", a);
    }

    /* local variable definition */
    a = 100;

    /* check the boolean condition */
    if (a < 20) {
        /* if condition is true then print the following */
        printf("a is less than 20\n");
    } else {
        /* if condition is false then print the following */
        printf("a is not less than 20\n");
    }

    return 0;
}

int main() {

    int a;

    /* for loop execution */
    for (a = 1; a < 10; a++) {
        printf("value of a: %d\n", a);
    }

    a = test_for();
    printf("value of a: %d\n", a);

    return 0;
}
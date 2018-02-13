//
// Created by tzt77 on 2/12/18.
//

#include <stdio.h>
#include <stdlib.h>

int add_j(int j) {
    int a = j + 1;

    int b = 11;

    if (j > 0) {

        b -= 10;

    } else {

        b += 12;

    }

    return a;
}

int test_sum(int length) {

    int j;

    int sum = 0;

    for (j = 0; j < length; j = add_j(j)) {
        sum += 1;
    }

    return sum;
}

int test_recu(int n) {

    if (n == 0 || n == 1)

        return 1;

    else
        return  test_recu(n - 1) + test_recu(n - 2);
}


int main(int argc, char **argv) {

    int iNum = atoi(argv[1]);
    int sum = 0;
    int i, j;

    for (i = 0; i < iNum; i++) {
        sum += test_sum(i);
    }

    test_recu(20);

    printf("%d\n", sum);
    return 0;
}
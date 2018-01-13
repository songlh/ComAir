#include "AProfHooks.h"

#include <stdio.h>


int main () {

    aprof_init();

    int a;

    /* for loop execution */
    for( a = 10; a < 20; a = a + 1 ){
        aprof_write(&a, sizeof(a));
//        printf("numbers = %p\n",(&a));
//        printf("numbers = %p\n",(&a) + 1);
//        printf("value of a: %d\n", a);
    }


    return 0;
}
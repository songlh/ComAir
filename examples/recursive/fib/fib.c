#include <stdlib.h>
#include <stdio.h>

long fib(int n) {

    if (n == 1 || n == 0) return 1;
    else return fib(n - 1) + fib(n - 2);
}

/*Benchmark entrance called by harness*/
//int app_main(int argc, char **argv) {
int main(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: fib [n]\n");
        exit(0);
    }

    int n = atoi(argv[1]);

    printf("%ld\n", fib(n));

    return 0;
}
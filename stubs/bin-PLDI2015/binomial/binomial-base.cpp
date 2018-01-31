/**********************************************************/
/* This code is for PLDI-15 Artifact Evaluation only      */
/* and will be released with further copyright information*/
/* File: Basic sequential recursive version of binomial   */
/**********************************************************/

#include <iostream>
#include <fstream>
#include <stdlib.h>
//#include "harness.h"

#ifdef TRACK_TRAVERSALS
uint64_t num_traversals = 0;
#endif

using namespace std;

void binomial(char n, char k, int *num) {

#ifdef TRACK_TRAVERSALS
    num_traversals++;
#endif

    if (k == 0 || k == n) {

        *num += 1;

    } else if (k < 0 || k > n) {

    } else {
        binomial(n - 1, k - 1, num);
        binomial(n - 1, k, num);
    }
}

/*Benchmark entrance called by harness*/
//int app_main(int argc, char **argv) {
int main(int argc, char **argv) {
    if (argc != 3) {
        cout << "usage: binomial [n] [k]" << endl;
        exit(0);
    }

    char n = atoi(argv[1]);
    char k = atoi(argv[2]);
    int num = 0;

    //Harness::start_timing();
    binomial(n, k, &num);
    cout << num << endl;
    //Harness::stop_timing();

#ifdef TRACK_TRAVERSALS
    cout << "num_traversals: " << num_traversals << endl;
#endif

    return 0;
}

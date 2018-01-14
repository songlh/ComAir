/**********************************************************/
/* This code is for PLDI-15 Artifact Evaluation only      */ 
/* and will be released with further copyright information*/ 
/* File: Basic sequential recursive version of fibonacci  */
/**********************************************************/

#include <iostream>
#include <fstream>
#include <stdlib.h>

//#include "harness.h"

#ifdef BLOCK_PROFILE
#include "blockprofiler.h"
BlockProfiler profiler; //simd utilization profiler
#endif

using namespace std;

int fib(int n) {
#ifdef BLOCK_PROFILE
  profiler.record_single();
#endif

  if (n == 1 || n == 0) return 1;
  else return fib(n - 1) + fib(n - 2);
}

/*Benchmark entrance called by harness*/
//int app_main(int argc, char **argv) {
int main(int argc, char** argv) {
  if (argc != 2) {
    cout << "usage: fib [n]" << endl;
    exit(0);
  }

  int n = atoi(argv[1]);

  //Harness::start_timing();
  cout << fib(n) << endl;
  //Harness::stop_timing();

#ifdef BLOCK_PROFILE
  profiler.output();//the simd utilization should be 0
#endif

  return 0;
}

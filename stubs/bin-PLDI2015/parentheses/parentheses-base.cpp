/**********************************************************/
/* This code is for PLDI-15 Artifact Evaluation only      */ 
/* and will be released with further copyright information*/ 
/* File: Basic sequential recursive version of parentheses*/
/**********************************************************/

#include <iostream>
#include <fstream>
#include <stdlib.h>
//#include "harness.h"

#ifdef BLOCK_PROFILE
#include "blockprofiler.h"
BlockProfiler profiler;//simd utilization profiler
#endif

using namespace std;

void parentheses(int l, int r, int n, int* num){
#ifdef BLOCK_PROFILE
	profiler.record_single();
#endif
  if (l == n){
    *num += 1;
    return;
  }

  parentheses(l + 1, r, n, num);
  if (l > r){
    parentheses(l, r + 1, n, num);
  }
}

/*Benchmark entrance called by harness*/
//int app_main(int argc, char** argv){
int main(int argc, char ** argv) {
  if (argc != 2){
    cout << "Usage: parentheses [n]" << endl;
    exit(0);
  }

  int n = atoi(argv[1]);
  int num = 0;

  //Harness::start_timing();
  parentheses(0, 0, n, &num);
  //Harness::stop_timing();

#ifdef BLOCK_PROFILE
  profiler.output();//the simd utilization should be 0
#endif
 
  cout << num << endl;

  return 0;
}

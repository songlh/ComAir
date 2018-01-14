/**********************************************************/
/* This code is for PLDI-15 Artifact Evaluation only      */ 
/* and will be released with further copyright information*/ 
/* File: Basic sequential recursive version of nqueens    */
/**********************************************************/

#include <iostream>
#include <fstream>
#include <stdlib.h>
//#include "harness.h"

#ifdef BLOCK_PROFILE
#include "blockprofiler.h"
BlockProfiler profiler;//simd utilization profiler
#endif

#ifdef TRACK_TRAVERSALS
uint64_t work = 0;
#endif

using namespace std;

/*from bots and cilk benchmark set*/
int ok(char n, char *a) {
	for (int i = 0; i < n; i++) {
		char p = a[i];

		for (int j = i + 1; j < n; j++) {
			char q = a[j];
			if (q == p || q == p - (j - i) || q == p + (j - i))
				return 0;
		}
	}
	return 1;
}

void nqueens(char n, char j, char *a, int *num, int _callIndex) {
#ifdef TRACK_TRAVERSALS
	work++;
#endif
#ifdef BLOCK_PROFILE
	profiler.record_single();
#endif

	if (_callIndex != -1) {
		a[j - 1] = _callIndex;
		if (!ok(j, a)) return;
	}

	if (n == j) {
		*num += 1;
		return;
	}

	/* try each possible position for queen <j> */
	for (int i = 0; i < n; i++) {
		nqueens(n, j + 1, a, num, i);
	}
}

/*Benchmark entrance called by harness*/
//int app_main(int argc, char **argv) {
int main(int argc, char ** argv) {
	if (argc < 2) {
		printf("number of queens required\n");
		return 1;
	}
	if (argc > 2)
		printf("extra arguments being ignored\n");

	int n = atoi(argv[1]);
	printf("running queens %d\n", n);

	//Harness::start_timing();

	char *a = (char *)alloca(n * sizeof(char));
	int num = 0;
	nqueens(n, 0, a, &num, -1);

	//Harness::stop_timing();
#ifdef BLOCK_PROFILE
	profiler.output();
#endif
#ifdef TRACK_TRAVERSALS
	cout << "work: " << work << endl;
#endif

	printf("nqueens = %d\n", num);

	return 0;
}

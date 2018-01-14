/**********************************************************/
/* This code is for PLDI-15 Artifact Evaluation only      */ 
/* and will be released with further copyright information*/ 
/* File: Basic sequential recursive version of knapsack   */
/**********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "getoptions.h"
#include <string.h>
#include <algorithm>
#include <iostream>
#include <fstream>
//#include "harness.h"

#ifdef BLOCK_PROFILE
#include "blockprofiler.h"
BlockProfiler profiler;
#endif

#ifdef TRACK_TRAVERSALS
uint64_t work = 0;
#endif

using namespace std;

/* every item in the knapsack has a weight and a value */
#define MAX_ITEMS 256

struct item {
  int value;
  int weight;
};

int best_so_far = INT_MIN;

int compare(struct item *a, struct item *b)
{
  double c = ((double) a->value / a->weight) -
      ((double) b->value / b->weight);

  if (c > 0)
    return -1;
  if (c < 0)
    return 1;
  return 0;
}

int read_input(const char *filename, struct item *items, int *capacity, int *n)
{
  int i;
  FILE *f;

  if (filename == NULL)
    filename = "\0";
  f = fopen(filename, "r");
  if (f == NULL) {
    fprintf(stderr, "open_input(\"%s\") failed\n", filename);
    return -1;
  }
  /* format of the input: #items capacity value1 weight1 ... */
  fscanf(f, "%d", n);
  fscanf(f, "%d", capacity);

  for (i = 0; i < *n; ++i)
    fscanf(f, "%d %d", &items[i].value, &items[i].weight);

  fclose(f);

  /* sort the items on decreasing order of value/weight */
  /* cilk2c is fascist in dealing with pointers, whence the ugly cast */
  qsort(items, *n, sizeof(struct item),
        (int (*)(const void *, const void *)) compare);

  return 0;
}

/*
 * return the optimal solution for n items (first is e) and
 * capacity c. Value so far is v.
 */
void knapsack(struct item *e, int c, int n, int v, int* max_ret)
{
#ifdef TRACK_TRAVERSALS
  work++;
#endif
#ifdef BLOCK_PROFILE
  profiler.record_single();
#endif

  /* base case: full knapsack or no items */
  if (c < 0){
    *max_ret = max(*max_ret, INT_MIN);
    return;
  }

  if (n == 0 || c == 0){
    *max_ret = max(*max_ret, v);
    return;		/* feasible solution, with value v */
  }

  /*
   * compute the best solution without the current item in the knapsack
   */
  knapsack(e + 1, c, n - 1, v, max_ret);

  /* compute the best solution with the current item in the knapsack */
  knapsack(e + 1, c - e->weight, n - 1, v + e->value, max_ret);
}

int usage(void)
{
  fprintf(stderr, "\nUsage: knapsack [<cilk-options>] [-f filename] [-benchmark] [-h]\n\n");
  fprintf(stderr, "The 0-1-Knapsack is a standard combinatorial optimization problem: ``A\n");
  fprintf(stderr, "thief robbing a store finds n items; the ith item is worth v_i dollars\n");
  fprintf(stderr, "and weighs w_i pounds, where v_i and w_i are integers. He wants to take\n");
  fprintf(stderr, "as valuable a load as possible, but he can carry at most W pounds in\n");
  fprintf(stderr, "his knapsack for some integer W. What items should he take?''\n\n");
  return -1;
}

char *specifiers[] = {"-f", "-benchmark", "-h", 0};
int opt_types[] = {STRINGARG, BENCHMARK, BOOLARG, 0};

/*Benchmark entrance called by harness*/
//int app_main(int argc, char *argv[])
int main(int argc, char * argv[])
{
  struct item items[MAX_ITEMS];	/* array of items */
  int n, capacity, sol = 0, benchmark, help;
  char filename[100];

  /* standard benchmark options */
  strcpy(filename, "knapsack-example2.input");

  get_options(argc-1, argv+1, specifiers, opt_types, filename, &benchmark, &help);

  if (help)
    return usage();

  if (benchmark) {
    switch (benchmark) {
      case 1:		/* short benchmark options -- a little work */
        strcpy(filename, "knapsack-example1.input");
        break;
      case 2:		/* standard benchmark options */
        strcpy(filename, "knapsack-example2.input");
        break;
      case 3:		/* long benchmark options -- a lot of work */
        strcpy(filename, "knapsack-example3.input");
        break;
    }
  }
  if (read_input(filename, items, &capacity, &n))
    return 1;

  //Harness::start_timing();
  knapsack(items, capacity, n, 0, &sol);
  //Harness::stop_timing();
#ifdef BLOCK_PROFILE
  profiler.output();
#endif
#ifdef TRACK_TRAVERSALS
  cout << "work: " << work << endl;
#endif

  printf("\nExample: knapsack\n");
  printf("options: problem-file = %s\n\n", filename);
  printf("Best value is %d\n\n", sol);

  return 0;
}

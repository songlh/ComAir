/**********************************************************/
/* This code is for PLDI-15 Artifact Evaluation only      */ 
/* and will be released with further copyright information*/ 
/* File: Basic sequential recursive version of graphcol   */
/**********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
//#include "harness.h"

#ifdef BLOCK_PROFILE
#include "blockprofiler.h"
BlockProfiler profiler;
#endif

#ifdef TRACK_TRAVERSALS
uint64_t work = 0;
#endif

using namespace std;

#define _MM_ALIGNED_  __attribute__((aligned(16)))
char* DATAFILE = "data38N64E.col";
#define node_t char

_MM_ALIGNED_ node_t * adjacentNodes; //Graph Pointer, in adjacent matrix format

void readEdge(FILE *data, int numEdge, int numNode, int *edge)
{
  char tmp[50];
  for(int i = 0; i < numEdge; i++)
  {
    fscanf(data, "%s %d %d\n", tmp, &edge[i*2], &edge[i*2+1]);
    edge[i*2]--;
    edge[i*2+1]--;
  }
}

void init_graph(int numNode, int numEdge, int* edge){
  adjacentNodes = (node_t *)malloc(numNode*numNode*sizeof(node_t));
  memset(adjacentNodes, 0, numNode*numNode*sizeof(node_t));

  for (int t = 0; t < numEdge; t++){
    adjacentNodes[edge[t*2]*numNode + edge[t*2+1]]
        = adjacentNodes[edge[t*2+1]*numNode + edge[t*2]]
        = 1;
  }
}

void print_graph(int numNode){
  printf("This is the graph: \n");
  for (int i = 0; i < numNode; ++i){
    for (int j = 0; j < numNode; ++j){
      printf("%d ", adjacentNodes[i*numNode+j]);
    }
    printf("\n");
  }
}

bool ok(int nodeId, int ci, node_t* c, int numNode){
  for (int k = 0; k < nodeId; ++k){
    for (int i = k + 1; i <= nodeId; ++i){
      if (adjacentNodes[k * numNode + i] == 1 && c[k] == c[i]){
        return false;
      }
    }
  }
  return true;
}

void print_solution(node_t* c, int numNode){
  printf("This is a solution: \n");
  for (int i = 0; i < numNode; ++i){
    printf("%d ", c[i]);
  }
  printf("\n");
}

void color(int nodeId, int numNode, int numColor, node_t* c, int *numSolution, int _callIndex){
#ifdef TRACK_TRAVERSALS
  work++;
#endif
#ifdef BLOCK_PROFILE
  profiler.record_single();
#endif

  if (_callIndex != -1){
    c[nodeId - 1] = _callIndex;
    if (!ok(nodeId - 1, _callIndex, c, numNode)) return;
  }

  if (nodeId == numNode){
    *numSolution += 1;
#ifdef _DEBUG
    if (*numSolution % 1000 == 0) print_solution(c, numNode);
#endif
    return;
  }

  //For current node, try color from color id = 1
  for (int ci = 1; ci <= numColor; ++ci){
    color(nodeId + 1, numNode, numColor, c, numSolution, ci);
  }
}

/*Benchmark entrance called by harness*/
//int app_main(int argc, char** argv){
int main(int argc, char ** argv) {
  int numSolution = 0;

  if (argc != 2 && argc != 3){
    printf("Usage: ./graphcoloring [color numbers] or Usage: ./graphcoloring [color numbers] [graph]\n");
    exit(0);
  }

  int numColor = atoi(argv[1]);
  printf("Start %d-Color Process...", numColor);
  if (argc == 3) DATAFILE = argv[2];

  /*******Read input data**************/
  int numNode, numEdge;
  int *edge;
  FILE *data;
  char tmp1[50], tmp2[50];
  data = fopen(DATAFILE, "r");
  if(data == NULL) {
    printf("Open data file failed\n");
    exit(0);
  }
  fscanf(data, "%s %s %d %d\n", tmp1, tmp2, &numNode, &numEdge);
  printf("numNode = %d, numEdge =%d\n", numNode, numEdge);

  edge = (int *)malloc(numEdge*sizeof(int)*2);
  readEdge(data, numEdge, numNode, edge);
  fclose(data);
  /********Read input data end**********/

  //Construct adjacent Matrix
  init_graph(numNode, numEdge, edge);
#ifdef _DEBUG
  print_graph(numNode);
#endif
  //Color auxiliary array
  _MM_ALIGNED_ node_t* c = (node_t*)malloc(numNode * sizeof(node_t));
  memset(c, 0, numNode * sizeof(node_t));

  //Start coloring...
  //Harness::start_timing();
  color(0, numNode, numColor, c, &numSolution, -1);
  //Harness::stop_timing();

#ifdef BLOCK_PROFILE
  profiler.output();
#endif
#ifdef TRACK_TRAVERSALS
  cout << "work: " << work << endl;
#endif

  printf("This is the number of possible solutions: %d\n", numSolution);

  free(edge);
  free(adjacentNodes);
  free(c);

  return 1;
}

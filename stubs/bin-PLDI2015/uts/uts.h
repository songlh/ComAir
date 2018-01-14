/*
 *         ---- The Unbalanced Tree Search (UTS) Benchmark ----
 *
 *  This file is part of the unbalanced tree search benchmark.  This
 *  project is licensed under the MIT Open Source license.  See the LICENSE
 *  file for copyright and licensing information.
 *
 *  UTS is a collaborative project between researchers at the University of
 *  Maryland, the University of North Carolina at Chapel Hill, and the Ohio
 *  State University.  See AUTHORS file for more information.
 *
 *  ** THIS IS A PRE-RELEASE VERSION OF UTS. **
 */

#ifndef _UTS_H
#define _UTS_H

#include "brg_sha1.h"
#include "head.h"

#define UTS_VERSION "2.1"

/***********************************************************
 *  Tree node descriptor and statistics                    *
 ***********************************************************/

#define MAXNUMCHILDREN    100  // cap on children (BIN root is exempt)


/* Tree  parameters */
extern double     b_0;
extern int        rootId;
extern int        nonLeafBF;
extern double     nonLeafProb;

/* Benchmark parameters */
extern int    computeGranularity;
extern int    debug;
extern int    verbose;

/* Utility Functions */
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

void serTreeSearch(int depth, Node *parent, int numChildren, unsigned long long* subtreesize);

int    uts_paramsToStr(char *strBuf, int ind);
void   uts_read_file(char *file);
void   uts_print_params();

/* Common tree routines */
void   uts_initRoot(Node * root);
int    uts_numChildren(Node *parent);
int    uts_numChildren_bin(Node * parent);
int    uts_numChildren_geo(Node * parent);
int    uts_childType(Node *parent);

void uts_show_stats( void );
int uts_check_result ( void );

#endif /* _UTS_H */

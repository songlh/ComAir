#ifndef COMAIR_LOOP_H
#define COMAIR_LOOP_H

#include "llvm/Pass.h"
#include "llvm/Analysis/LoopInfo.h"

using namespace llvm;
using namespace std;

void LoopSimplify(Loop * pLoop, DominatorTree *DT);

std::set<Loop *> getSubLoopSet(Loop *lp);

std::set<Loop *> getLoopSet(Loop *lp);

#endif //COMAIR_LOOP_H

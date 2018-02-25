#ifndef COMAIR_LOOP_H
#define COMAIR_LOOP_H

#include "llvm/Pass.h"
#include "llvm/Analysis/LoopInfo.h"

using namespace llvm;

void LoopSimplify(Loop * pLoop, DominatorTree *DT);

#endif //COMAIR_LOOP_H

#ifndef COMAIR_INVARRAY_H
#define COMAIR_INVARRAY_H

#include <set>
#include <vector>

#include "llvm/Pass.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/DataLayout.h"

using namespace std;
using namespace llvm;

struct InvArray: public ModulePass
{
    static char ID;
    InvArray();
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;
    virtual bool runOnModule(Module &M);
    virtual void print(raw_ostream &O, const Module *M) const;

private:
    void SetupInit(Module &M);
    void CollectAllLoop(Function *F);
    bool IsArrayAccessLoop(BasicBlock * pHeader, set<LoadInst *> & setArrayLoad);
    void AnalyzeLoopAccess(Function *F, set<LoadInst *> & setArrayLoad);

private:

    Module * pModule;

    ScalarEvolution * SE;
    DataLayout * DL;

    std::set<Loop *> LoopSet;

};

#endif //COMAIR_INVARRAY_H

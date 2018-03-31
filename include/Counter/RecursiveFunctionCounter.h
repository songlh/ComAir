#ifndef COMAIR_RECURSIVEFUNCTIONCOUNTER_H
#define COMAIR_RECURSIVEFUNCTIONCOUNTER_H

#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"


using namespace llvm;

struct RecursiveFunctionCounter : public ModulePass
{
    static char ID;
    RecursiveFunctionCounter();

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;
    virtual bool runOnModule(Module& M);

};

#endif //COMAIR_RECURSIVEFUNCTIONCOUNTER_H

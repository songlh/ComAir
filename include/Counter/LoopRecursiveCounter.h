#ifndef COMAIR_LoopRecursiveCounter_H
#define COMAIR_LoopRecursiveCounter_H

#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Analysis/LoopInfo.h"

using namespace llvm;

struct LoopRecursiveCounter : public ModulePass
{
    static char ID;
    LoopRecursiveCounter();
    Module *pModule;

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;
    virtual bool runOnModule(Module& M);

    void SetupTypes(Module *);

    void SetupFunctions(Module *);

    void InstrumentInit(Instruction *);

    void InstrumentRecursiveFunc(Function *);

    void InstrumentLoop(Loop *);

    void InstrumentDump(Instruction *);


//type
    IntegerType *LongType;

    IntegerType *IntType;

    Type *VoidType;

    // functions
    Function *aprof_init;

    Function *updater;

    Function *dump;

};

#endif //COMAIR_LoopRecursiveCounter_H

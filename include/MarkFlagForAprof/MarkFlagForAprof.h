#ifndef COMAIR_MARKFLAGFORAPROF_H
#define COMAIR_MARKFLAGFORAPROF_H

#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"


using namespace llvm;

struct MarkFlagForAprof : public ModulePass {
    static char ID;

    MarkFlagForAprof();

    Module *pModule;

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

    virtual bool runOnModule(Module &M);

    void MarkFlag(BasicBlock *, int);
    void MarkFlag(Instruction *, int);

    void MarkBBUpdateFlag(Function *);
    void MarkInstFlag(Instruction *);

    void OptimizeReadWrite();
    void NotOptimizeReadWrite();

    void setupInit(Module *M);
    void setupTypes();

    // Type
    Type *IntType;
};

#endif //COMAIR_MARKFLAGFORAPROF_H
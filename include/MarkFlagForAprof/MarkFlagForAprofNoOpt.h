#include "llvm/Pass.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"

using namespace llvm;

struct MarkFlagForAprofNoOpt : public ModulePass {
    static char ID;
    MarkFlagForAprofNoOpt();
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;
    virtual bool runOnModule(Module &M);



    void markFlag(Instruction * Inst, int Flag);

    // Type
private:
    Type *IntType;
    Module * pModule;
};
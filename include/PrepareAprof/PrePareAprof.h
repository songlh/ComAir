#ifndef COMAIR_PREPAREAPROF_H
#define COMAIR_PREPAREAPROF_H


#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"


using namespace llvm;
using namespace std;

struct PrepareAprof : public ModulePass {

    static char ID;

    PrepareAprof();

    Module *pModule;

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

    virtual bool runOnModule(Module &M);

    void SetupInit(Module *);

    void SetupTypes(Module *);

    void SetupConstants(Module *);

    void SetupGlobals(Module *);

    BinaryOperator *CreateIfElseBlock(Function *, Module *,  std::vector<BasicBlock *> &);

    void RemapInstruction(Instruction *I, ValueToValueMapTy &VMap);

    void CloneTargetFunction(Function *, std::vector<BasicBlock *> &, ValueToValueMapTy &);

    void AddSwitcher(Function *);

    // type
    IntegerType * LongType;

    //global
    GlobalVariable *Switcher;

    //constant
    ConstantInt *ConstantLong0;
    ConstantInt *ConstantLong1;

    // constant  -1
    ConstantInt *ConstantLongN1;
};


#endif //COMAIR_PREPAREAPROF_H

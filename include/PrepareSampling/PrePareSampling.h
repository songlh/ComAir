#ifndef COMAIR_PREPAREAPROF_H
#define COMAIR_PREPAREAPROF_H


#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"


using namespace llvm;
using namespace std;

struct PrepareSampling : public ModulePass {

    static char ID;

    PrepareSampling();

    Module *pModule;

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

    virtual bool runOnModule(Module &M);

    void SetupTypes(Module *);

    void SetupConstants(Module *);

    void SetupGlobals(Module *);

    void SetupFunctions(Module *);

    void SetupInit(Module *);

    BinaryOperator *CreateIfElseBlock(Function *, Module *,  std::vector<BasicBlock *> &);

    void RemapInstruction(Instruction *I, ValueToValueMapTy &VMap);

    void CloneTargetFunction(Function *, std::vector<BasicBlock *> &, ValueToValueMapTy &);

    void AddSwitcher(Function *);

    void CloneFunctionCalled();

    // type
    IntegerType * LongType;
    IntegerType * IntType;

    //global
    GlobalVariable *Switcher;
    GlobalVariable *GeoRate;

    //constant
    ConstantInt *ConstantInt0;
    ConstantInt *ConstantBigInt;
    ConstantInt *ConstantLong0;
    ConstantInt *ConstantLong1;

    // constant  -1
    ConstantInt *ConstantIntN1;

    // function geo
    Function *geo;
};


#endif //COMAIR_PREPAREAPROF_H

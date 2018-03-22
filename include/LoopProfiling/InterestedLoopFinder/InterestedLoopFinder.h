#ifndef COMAIR_INTERESTEDLOOPFINDER_H
#define COMAIR_INTERESTEDLOOPFINDER_H

#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"


using namespace llvm;

struct InterestedLoopFinder : public ModulePass {
    static char ID;

    Module *pModule;

    InterestedLoopFinder();


    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

    virtual bool runOnModule(Module &M);

    void SetupInit(Module *);

    void SetupTypes(Module *);

    void SetupConstants(Module *);

    void SetupGlobals(Module *);

    void SetupFunctions(Module *);

    void InstrumentLoop(Loop *);

    void InstrumentLoopIn(Loop *);

    void InstrumentLoopOut(Loop *);

    void InstrumentInit(Instruction *);

//type
    IntegerType *LongType;

    IntegerType *IntType;

    Type *VoidType;


//function
    Function *aprof_init;

    Function *aprof_loop_in;

    Function *aprof_loop_out;


//global
    GlobalVariable *numCost;

//constant
    ConstantInt *ConstantLong0;
    ConstantInt *ConstantLong1;
};


#endif //COMAIR_INTERESTEDLOOPFINDER_H

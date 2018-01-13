#ifndef COMAIR_APROFHOOKPASS_H
#define COMAIR_APROFHOOKPASS_H

#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"


using namespace llvm;

struct AprofHook : public ModulePass
{
    static char ID;
    AprofHook();

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;
    virtual bool runOnModule(Module& M);

    void SetupInit(Module *);
    void SetupTypes(Module *);
    void SetupConstants(Module *);
    void SetupGlobals(Module *);
    void SetupFunctions(Module *);
    void SetupHooks(Module *);

    void InsertAprofInit(Instruction *);
    void InsertAProfIncrementCost(Instruction *);

    //type
    IntegerType * IntType;
    IntegerType * LongType;
    Type * VoidType;

    /* function */

    // int aprof_init()
    Function * aprof_init;

    Function * aprof_increment_cost;

    //global
    GlobalVariable * aprof_count;
    GlobalVariable * aprof_bb_count;

    //constant
    ConstantInt * ConstantLong0;
    ConstantInt * ConstantLong1;

};

#endif //COMAIR_APROFHOOKPASS_H

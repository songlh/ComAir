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

    void SetupInit();
    void SetupTypes();
    void SetupConstants();
    void SetupGlobals();
    void SetupFunctions();
    void SetupHooks();

    void InsertAprofInit(Instruction *);
    void InsertAProfIncrementCost(Instruction *);
    void InsertAprofWrite(Value *, Instruction *);

    // Module
    Module *pModule;

    //type
    IntegerType * IntType;
    IntegerType * LongType;
    Type * VoidType;
    PointerType* VoidPointerType;

    /* function */

    // int aprof_init()
    Function * aprof_init;
    // void aprof_increment_cost()
    Function * aprof_increment_cost;
    // void aprof_write(unsigned long start_addr, unsigned int length)
    Function *aprof_write;

    //global
    GlobalVariable * aprof_count;
    GlobalVariable * aprof_bb_count;

    //constant
    ConstantInt * ConstantLong0;
    ConstantInt * ConstantLong1;

};

#endif //COMAIR_APROFHOOKPASS_H

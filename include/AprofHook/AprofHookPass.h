#ifndef COMAIR_APROFHOOKPASS_H
#define COMAIR_APROFHOOKPASS_H

#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"


using namespace llvm;
using namespace std;


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

    void InstrumentCostUpdater(BasicBlock *);

    void InsertAprofInit(Instruction *);
    void InsertAprofIncrementCost(Instruction *);
    void InsertAprofIncrementRms(Instruction *);
    void InsertAprofWrite(Value *, Instruction *);
    void InsertAprofRead(Value *, Instruction *);
    void InsertAprofAlloc(Value *, Instruction *);
    void InsertAprofCallBefore(int FuncID, Instruction *BeforeCallInst);
    void InsertAprofReturn(Instruction *);

    void InstrumentCostUpdater(Function * pFunction);

    /* Module */
    Module *pModule;
    /* ********** */

    /* Type */
    IntegerType * IntType;
    IntegerType * LongType;
    Type * VoidType;
    PointerType* VoidPointerType;
    /* ********** */

    /* Function */
    // int aprof_init()
    Function * aprof_init;
    // void aprof_increment_cost()
    Function * aprof_increment_cost;
    // void aprof_increment_rms
    Function *aprof_increment_rms;
    // void aprof_write(void *memory_addr, unsigned int length)
    Function *aprof_write;
    // void aprof_read(void *memory_addr, unsigned int length)
    Function *aprof_read;
    // void aprof_call_before(char *funcName)
    Function *aprof_call_before;
    // void aprof_return()
    Function *aprof_return;
    /* ********** */

    /* Global Variable */
    GlobalVariable * aprof_count;
    GlobalVariable * aprof_bb_count;
    /* ********** */

    /* Constant */
    ConstantInt * ConstantLong0;
    ConstantInt * ConstantLong1;
    /* ********** */

    /* */
    AllocaInst *BBAllocInst;
    /* **** */

};

#endif //COMAIR_APROFHOOKPASS_H

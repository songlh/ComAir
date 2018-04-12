#ifndef COMAIR_RECURSIVERMSHOOK_H
#define COMAIR_RECURSIVERMSHOOK_H

#ifndef COMAIR_LOOPINTERESTEDHOOK_H
#define COMAIR_LOOPINTERESTEDHOOK_H

#include <fstream>

#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"


using namespace llvm;
using namespace std;


struct RecursiveRmsHook : public ModulePass {
    static char ID;

    RecursiveRmsHook();

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

    virtual bool runOnModule(Module &M);

    bool hasRecursiveCall(Module &M);

    void SetupInit();

    void SetupTypes();

    void SetupConstants();

    void SetupGlobals();

    void SetupFunctions();

    void SetupHooks();

    void SetupSampleRecursiveHooks();

    void InstrumentInit(Instruction *);

    void InstrumentRead(LoadInst *, Instruction *);

    void InstrumentWrite(StoreInst *, Instruction *);

    void InstrumentHooks(Function *);

    void InstrumentCallBefore(Function *pFunction);

    void InstrumentRmsUpdater(Function *F);

    void InstrumentUpdater(Function *pFunction);

    void InstrumentReturn(Instruction *);

    void InstrumentFinal(Function *);

    /* Module */
    Module *pModule;
    /* ********** */

    /* Type */
    IntegerType *IntType;
    IntegerType *LongType;
    Type *VoidType;
    PointerType *VoidPointerType;
    /* ********** */

    /* Function */
    // int aprof_init()
    Function *aprof_init;
    // void aprof_increment_cost()
    Function *aprof_increment_cost;
    // void aprof_increment_rms
    Function *aprof_increment_rms;
    // void aprof_increment_rms
    Function *aprof_increment_rms_for_args;
    // void aprof_write(void *memory_addr, unsigned int length)
    Function *aprof_write;
    // void aprof_read(void *memory_addr, unsigned int length)
    Function *aprof_read;
    // void aprof_call_before(char *funcName)
    Function *aprof_call_before;
    // void aprof_return()
    Function *aprof_return;
    // void aprof_final()
    Function *aprof_final;
    /* ********** */

    /* Constant */
    ConstantInt *ConstantLong0;
    ConstantInt *ConstantLong1;
    ConstantInt *ConstantLongN1;
    /* ********** */

    /* Global Variable */
    GlobalVariable *numCost;
    GlobalVariable *callStack;
    /* **** */

    /* Temp Alloc Inst */
    AllocaInst *TNumCost;

    /* OutPut File */
    ofstream funNameIDFile;
    /* */

};

#endif //COMAIR_LOOPINTERESTEDHOOK_H


#endif //COMAIR_RECURSIVERMSHOOK_H

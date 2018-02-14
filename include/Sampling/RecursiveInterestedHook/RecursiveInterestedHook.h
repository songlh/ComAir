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


struct RecursiveInterestedHook : public ModulePass {
    static char ID;

    RecursiveInterestedHook();

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

    virtual bool runOnModule(Module &M);

    void SetupInit();

    void SetupTypes();

    void SetupConstants();

    void SetupGlobals();

    void SetupFunctions();

    void CollectInterestedLoopFunction();

    void SetupHooks();

    void InstrumentInit(Instruction *);

    void InstrumentHooks(Function *);


    void InstrumentCallBefore(Function *pFunction);

    void InstrumentReturn(Instruction *m);

    void InstrumentCostUpdater(Function *pFunction);

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
    // void aprof_call_before(char *funcName)
    Function *aprof_call_before;
    // void aprof_return()
    Function *aprof_return;
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

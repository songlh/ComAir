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

    void InstrumentInit(Instruction *);

    void InstrumentRead(LoadInst *, Instruction *);

    void InstrumentHooks(Function *);

    void InstrumentCallBefore(Function *pFunction);

    void InstrumentReturn(Instruction *m);

    void InstrumentUpdater(Function *pFunction);

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
    // void aprof_read(void *memory_addr, unsigned long length)
    Function *aprof_read;
    // void aprof_call_before(char *funcName)
    Function *aprof_call_in;
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


#endif //COMAIR_RECURSIVERMSHOOK_H

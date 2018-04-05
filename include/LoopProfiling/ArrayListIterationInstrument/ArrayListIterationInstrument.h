#ifndef COMAIR_ARRAYLISTINSTRUMENT_H
#define COMAIR_ARRAYLISTINSTRUMENT_H

#include "llvm/Pass.h"

using namespace llvm;
using namespace std;

struct ArrayListIterationInstrument : public ModulePass {

    static char ID;

    ArrayListIterationInstrument();

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

    virtual bool runOnModule(Module &M);

    void SetupInit(Module &M);

    void SetupTypes();

    void SetupConstants();

    void SetupGlobals();

    void SetupFunctions();

    void SetupHooks();

    void InstrumentHooks(Function *, Instruction *);

    void InstrumentInit(Instruction *);

    void InstrumentCallBefore(Function *);

    void InstrumentCostUpdater(Loop *pLoop);

    void InstrumentReturn(Function *);

    void InstrumentFinal(Function *);

    /* Module */
    Module *pModule;
    /* ********** */

    /* Type */
    Type *VoidType;
    IntegerType *LongType;
    IntegerType *IntType;
    PointerType *VoidPointerType;
    /* ********** */

    /* Global Variable */
//    GlobalVariable *numCost;
    AllocaInst *itNum;
    /* ***** */

    /* Function */
    // int aprof_init()
    Function *aprof_init;

    Function *aprof_call_before;

    // aprof_read(void *memory_addr, int length)
    Function *aprof_read;
    // void aprof_return(unsigned long numCost,  unsigned long itNum)
    Function *aprof_return;

    Function *aprof_final;
    /* ********** */

    /* Constant */
    ConstantInt *ConstantLong0;
    ConstantInt *ConstantLong1;
    /* ********** */

    AllocaInst *BBAllocInst;

};

#endif //COMAIR_ARRAYLISTINSTRUMENT_H

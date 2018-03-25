#ifndef COMAIR_ARRAYLISTINSTRUMENT_H
#define COMAIR_ARRAYLISTINSTRUMENT_H

#include "llvm/Pass.h"

using namespace llvm;
using namespace std;

struct ArrayListInstrument : public ModulePass {

    static char ID;

    ArrayListInstrument();

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

    void InstrumentReturn(Instruction *returnInst);


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
    GlobalVariable *numCost;
    AllocaInst *itNum;
    /* ***** */

    /* Function */
    // int aprof_init()
    Function *aprof_init;
    // aprof_dump(void *memory_addr, int length)
    Function *aprof_dump;
    // void aprof_return(unsigned long numCost,  unsigned long itNum)
    Function *aprof_return;
    /* ********** */

    /* Constant */
    ConstantInt *ConstantLong0;
    ConstantInt *ConstantLong1;
    /* ********** */

};

#endif //COMAIR_ARRAYLISTINSTRUMENT_H

#ifndef COMAIR_ARRAYLISTINSTRUMENT_H
#define COMAIR_ARRAYLISTINSTRUMENT_H

#include "llvm/Pass.h"

using namespace llvm;
using namespace std;

struct ArrayListSampleInstrument : public ModulePass {

    static char ID;

    ArrayListSampleInstrument();

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

    virtual bool runOnModule(Module &M);

    void SetupInit(Module &M);

    void SetupTypes();

    void SetupConstants();

    void SetupGlobals();

    void SetupFunctions();

    void SetupHooks();

    void MarkFlag(Instruction *Inst);

    void InstrumentHooks(Function *, Instruction *);

    void InstrumentInit(Instruction *);

    void InstrumentReturn(Instruction *returnInst);

    void CreateIfElseIfBlock(Loop *pInnerLoop, vector<BasicBlock *> &vecAdded);

    void CloneInnerLoop(Loop *pLoop, vector<BasicBlock *> &vecAdd, ValueToValueMapTy &VMap);

    void RemapInstruction(Instruction *I, ValueToValueMapTy &VMap);

    void InstrumentCostUpdater(Loop *pLoop);

    void InstrumentInnerLoop(Loop * pInnerLoop, PostDominatorTree * PDT);


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
    GlobalVariable *Switcher;
    GlobalVariable *GeoRate;
    /* ***** */

    /* Function */
    // int aprof_init()
    Function *aprof_init;
    // aprof_dump(void *memory_addr, int length)
    Function *aprof_dump;
    // void aprof_return(unsigned long numCost,  unsigned long itNum)
    Function *aprof_return;

    Function *aprof_geo;
    /* ********** */

    /* Constant */
    ConstantInt *ConstantLong0;
    ConstantInt *ConstantLong1;
    ConstantInt *ConstantInt0;
    ConstantInt *ConstantInt1;
    ConstantInt *ConstantSamplingRate;
    /* ********** */

};

#endif //COMAIR_ARRAYLISTINSTRUMENT_H

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


struct LoopInterestedHook : public ModulePass {
    static char ID;

    LoopInterestedHook();

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

    virtual bool runOnModule(Module &M);

    void SetupInit();

    void SetupTypes();

    void SetupConstants();

    void SetupGlobals();

    void SetupFunctions();

    void SetupHooks();

    void InstrumentInit(Instruction *);

    void InstrumentInnerLoop(Loop * pInnerLoop, PostDominatorTree * PDT);

    void AddHooksToInnerFunction(Function *pInnerFunction);

    void AddHooksToOuterFunction(Function *pOuterFunction);

    void RemapInstruction(Instruction *I, ValueToValueMapTy &VMap);

    void CreateIfElseIfBlock(Loop * pInnerLoop, vector<BasicBlock *> & vecAdded);

    void CloneInnerLoop(Loop * pLoop, vector<BasicBlock *> & vecAdd, ValueToValueMapTy & VMap);

    void InstrumentHooks(Function *, bool isOptimized = true);

    void InstrumentRead(LoadInst *, Instruction *);

    void InstrumentWrite(StoreInst *, Instruction *);

    void InstrumentReturn(Instruction *m);

    void InstrumentCostUpdater(Loop *pLoop);

    void InstrumentOuterLoop(Loop *pOutLoop);

    void CloneFunctionCalled();

    bool bGivenOuterLoop;

    /* Module */
    Module *pModule;
    /* ********** */

    /* Type */
    IntegerType *IntType;
    IntegerType *LongType;
    Type *VoidType;
    PointerType *VoidPointerType;
    /* ********** */

    /* Global Variable */
    GlobalVariable *numCost;
    GlobalVariable *Switcher;
    GlobalVariable *GeoRate;
    GlobalVariable *SampleMonitor;
    /* ***** */

    /* Function */
    // int aprof_init()
    Function *aprof_init;

    // void aprof_read(void *memory_addr, unsigned long length, int sample)
    Function *aprof_read;
    // void aprof_read(void *memory_addr, unsigned long length, int sample)
    Function *aprof_write;
    // void aprof_return(unsigned long numCost, int sample)
    Function *aprof_return;

    Function *aprof_geo;
    /* ********** */

    /* Constant */
    ConstantInt *ConstantLong0;
    ConstantInt *ConstantInt0;
    ConstantInt *ConstantLong1;
    ConstantInt *ConstantInt1;
    ConstantInt *ConstantInt2;
    ConstantInt *ConstantInt3;
    ConstantInt *ConstantSamplingRate;
    /* ********** */

    /* OutPut File */
    ofstream funNameIDFile;
    /* */

    Loop * pOuterLoop;

    std::set<int> MonitoredInst;

};

#endif //COMAIR_LOOPINTERESTEDHOOK_H

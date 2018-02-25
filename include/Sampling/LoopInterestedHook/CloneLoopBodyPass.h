#ifndef COMAIR_CLONELOOPBODYPASS_H
#define COMAIR_CLONELOOPBODYPASS_H


#include <fstream>

#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"


using namespace llvm;
using namespace std;


struct CloneLoopBodyPass : public ModulePass {
    static char ID;

    CloneLoopBodyPass();

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

    virtual bool runOnModule(Module &M);

    void SetupInit();

    void SetupTypes(Module *pModule);

    void SetupConstants(Module *pModule);

    void SetupGlobals(Module *pModule);

    void SetupFunctions(Module *pModule);

    void SetupHooks();

    void InstrumentRead(LoadInst *, Instruction *);

    void RemapInstruction(Instruction *I, ValueToValueMapTy &VMap);

    void SplitHeader(Loop * pLoop);

    void InstrumentNewHeader();

    void UpdateHeader(set<BasicBlock *> & setCloned, ValueToValueMapTy & VMap);

    void UpdateExitBlock(Function * pFunction, set<BasicBlock *> & setExitBlocks, set<BasicBlock *> & setCloned, ValueToValueMapTy & VMap);

    void DoClone(Function * pFunction, vector<BasicBlock *> & ToClone, ValueToValueMapTy & VMap, set<BasicBlock *> & setCloned);

    void CloneLoopBody(Loop * pLoop, vector<Instruction *> & vecInst );

    void InstrumentPreHeader(Loop *pLoop);

    void AddHooksToInnerFunction(Function *pInnerFunction);

    bool bGivenOuterLoop;

    /* Module */
    Module *pM;
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
    ConstantInt *ConstantSamplingRate;
    ConstantInt *ConstantLong1000;
    /* ********** */

    /* OutPut File */
    ofstream funNameIDFile;
    /* */

    /* */
    BasicBlock * pPreHeader;
    BasicBlock * pHeader;
    BasicBlock * pOldHeader;
    BasicBlock * pNewHeader;

    //global
    GlobalVariable * SAMPLE_RATE;
    GlobalVariable * CURRENT_SAMPLE;
    GlobalVariable * numInstances;
    GlobalVariable * PC_SAMPLE_RATE;
    GlobalVariable * MAX_SAMPLE;
    GlobalVariable * Record_CPI;
    GlobalVariable * pcBuffer_CPI;
    GlobalVariable * iRecordSize_CPI;
    GlobalVariable * iBufferIndex_CPI;

    Constant * SAMPLE_RATE_ptr;
    Constant * Output_Format_String;

    //Instruction
    BinaryOperator * pAddCurrentInstances;
    LoadInst * pLoadCurrent_Sample;
    LoadInst * pLoadMAX_SAMPLE;
    CastInst * pCastCURRENT_SAMPLE;
    BinaryOperator * pAddCURRENT_SAMPLE;

};


#endif //COMAIR_CLONELOOPBODYPASS_H

#ifndef COMAIR_DUMPCALLSTACK_H
#define COMAIR_DUMPCALLSTACK_H

#include <fstream>

#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"


using namespace llvm;
using namespace std;


struct DumpCallStack : public ModulePass {
    static char ID;
    Module *pModule;

    DumpCallStack();

    // dump function list
    set<string> FuncNameList;

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;
    virtual bool runOnModule(Module &M);

    void SetupInit();
    void Instrument(Function *F);
    void InstrumentDumInit(Function *MainFunc);
    void InstrumentEnterStack(Function *F);
    void InstrumentOuterStack(Instruction *returnInst);
    void InstrumentDumpStack(Function *F);

    // function
    Function *DumpInit;
    Function *EnterStack;
    Function *OuterStack;
    Function *DumpStack;

    /* Type */
    Type *VoidType;
    IntegerType *IntType;
};

#endif //COMAIR_DUMPCALLSTACK_H

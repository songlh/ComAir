#include <vector>
#include <sstream>
#include <set>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "DumpCallStack/DumpCallStack.h"
#include "Common/Helper.h"

using namespace std;

static RegisterPass<DumpCallStack> X("dump-call-stack", "dump call stack", true, true);

static cl::opt<std::string> strFileName(
        "strFileName",
        cl::desc("The name of File to read complexity function names"), cl::Optional,
        cl::value_desc("strFileName"));


char DumpCallStack::ID = 0;

void DumpCallStack::getAnalysisUsage(AnalysisUsage &AU) const {

}


DumpCallStack::DumpCallStack() : ModulePass(ID) {

}


void DumpCallStack::InstrumentDumInit(Function *MainFunc) {
    Instruction *firstInst = &*(MainFunc->getEntryBlock().getFirstInsertionPt());
    std::vector<Value *> vecParams;
    CallInst *void_49 = CallInst::Create(this->DumpInit, vecParams, "", firstInst);
    void_49->setCallingConv(CallingConv::C);
    void_49->setTailCall(false);
    AttributeList void_PAL;
    void_49->setAttributes(void_PAL);
}

void DumpCallStack::InstrumentEnterStack(Function *F) {
    int FuncID = GetFunctionID(F);
    std::vector<Value *> vecParams;
    Instruction *firstInst = &*(F->getEntryBlock().getFirstInsertionPt());

    ConstantInt *const_int6 = ConstantInt::get(
            this->pModule->getContext(),
            APInt(32, StringRef(std::to_string(FuncID)), 10));

    vecParams.push_back(const_int6);

    CallInst *void_46 = CallInst::Create(
            this->EnterStack, vecParams, "", firstInst);
    void_46->setCallingConv(CallingConv::C);
    void_46->setTailCall(false);
    AttributeList void_PAL;
    void_46->setAttributes(void_PAL);
}

void DumpCallStack::InstrumentOuterStack(Instruction *returnInst) {
    std::vector<Value *> vecParams;
    CallInst *void_49 = CallInst::Create(this->OuterStack, vecParams, "", returnInst);
    void_49->setCallingConv(CallingConv::C);
    void_49->setTailCall(false);
    AttributeList void_PAL;
    void_49->setAttributes(void_PAL);
}

void DumpCallStack::InstrumentDumpStack(Function *F) {
    Instruction *firstInst = &*(F->getEntryBlock().getFirstInsertionPt());
    std::vector<Value *> vecParams;
    CallInst *void_49 = CallInst::Create(this->DumpStack, vecParams, "", firstInst);
    void_49->setCallingConv(CallingConv::C);
    void_49->setTailCall(false);
    AttributeList void_PAL;
    void_49->setAttributes(void_PAL);
}


void DumpCallStack::Instrument(Function *F) {

    InstrumentEnterStack(F);

    for (Function::iterator BI = F->begin(); BI != F->end(); BI++) {

        BasicBlock *BB = &*BI;

        for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {

            Instruction *Inst = &*II;

            switch (Inst->getOpcode()) {

                case Instruction::Ret: {
                    InstrumentOuterStack(Inst);
                }
            }
        }
    }
}

void DumpCallStack::SetupInit() {

    // init
    this->VoidType = Type::getVoidTy(pModule->getContext());
    this->IntType = IntegerType::get(pModule->getContext(), 32);

    std::vector<Type *> ArgTypes;

    // dump_init
    this->DumpInit = this->pModule->getFunction("dump_init");

    if (!this->DumpInit) {
        FunctionType *DumpInitType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->DumpInit = Function::Create(DumpInitType, GlobalValue::ExternalLinkage, "dump_init", this->pModule);
        ArgTypes.clear();
    }

    // Enter Stack Function
    this->EnterStack = this->pModule->getFunction("enter_stack");

    if (!this->EnterStack) {
        ArgTypes.push_back(this->IntType);
        FunctionType *EnterStackType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->EnterStack = Function::Create(EnterStackType, GlobalValue::ExternalLinkage, "enter_stack", this->pModule);
        ArgTypes.clear();
    }

    // Outer Stack Function
    this->OuterStack = this->pModule->getFunction("outer_stack");

    if (!this->OuterStack) {
        FunctionType *EnterStackType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->OuterStack = Function::Create(EnterStackType, GlobalValue::ExternalLinkage, "outer_stack", this->pModule);
        ArgTypes.clear();
    }

    // Dump Stack Function
    this->DumpStack = this->pModule->getFunction("dump_stack");

    if (!this->DumpStack) {
        FunctionType *EnterStackType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->DumpStack = Function::Create(EnterStackType, GlobalValue::ExternalLinkage, "dump_stack", this->pModule);
        ArgTypes.clear();
    }
}

bool DumpCallStack::runOnModule(Module &M) {

    this->pModule = &M;
    set<string> FuncNamesList;

    if (strFileName.empty()) {
        errs() << "Please set file name!" << "\n";
        exit(1);
    }

    SetupInit();

    std::ifstream input(strFileName);
    for (std::string line; getline(input, line);) {
        FuncNamesList.insert(line);
    }

    if (FuncNamesList.empty()) {
        errs() << "Please add some dump function names!" << "\n";
        exit(1);
    }

    // instrument dump stack
    for (auto it = FuncNamesList.begin(); it != FuncNamesList.end(); it++) {

        Function *F = this->pModule->getFunction(*it);
        if (F) {
            InstrumentDumpStack(F);
        } else {
            assert(F != NULL);
        }
    }

    // instrument enter and outer function
    for (Module::iterator FI = this->pModule->begin(); FI != this->pModule->end(); FI++) {

        Function *Func = &*FI;
        if (IsIgnoreFunc(Func))
            continue;

        Instrument(Func);
    }

    // find main instrument init
    Function *MainFunc = this->pModule->getFunction("main");
    assert(MainFunc != NULL);

    if (MainFunc) {
        InstrumentDumInit(MainFunc);
    }
    return false;
}

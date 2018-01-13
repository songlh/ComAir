#include "AprofHook/AprofHookPass.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"


using namespace llvm;
using namespace std;


static RegisterPass<AprofHook> X(
        "algo-profiling",
        "profiling algorithmic complexity",
        true, true);


char AprofHook::ID = 0;


void AprofHook::getAnalysisUsage(AnalysisUsage &AU) const {}

AprofHook::AprofHook() : ModulePass(ID) {}

void AprofHook::SetupTypes(Module *pModule) {
    this->VoidType = Type::getVoidTy(pModule->getContext());
    this->IntType = IntegerType::get(pModule->getContext(), 32);
    this->LongType = IntegerType::get(pModule->getContext(), 64);
}

void AprofHook::SetupConstants(Module *pModule) {
    this->ConstantLong0 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("0"), 10));
    this->ConstantLong1 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("1"), 10));
}

void AprofHook::SetupGlobals(Module *pModule) {

    assert(pModule->getGlobalVariable("aprof_count") == NULL);
    this->aprof_count = new GlobalVariable(
            *pModule, this->LongType, false,
            GlobalValue::ExternalLinkage, 0,
            "aprof_count");
    this->aprof_count->setAlignment(8);
    this->aprof_count->setInitializer(this->ConstantLong0);

    assert(pModule->getGlobalVariable("aprof_bb_count") == NULL);
    this->aprof_bb_count = new GlobalVariable(
            *pModule, this->LongType, false,
            GlobalValue::ExternalLinkage, 0,
            "aprof_bb_count");
    this->aprof_bb_count->setAlignment(8);
    this->aprof_bb_count->setInitializer(this->ConstantLong0);
}

void AprofHook::SetupFunctions(Module *pModule) {
    std::vector<Type *> ArgTypes;
    FunctionType *AprofInitType = FunctionType::get(this->IntType, ArgTypes, false);
    this->aprof_init = Function::Create
            (AprofInitType, GlobalValue::ExternalLinkage, "aprof_init", pModule);


    FunctionType *AprofIncrementCostType = FunctionType::get(this->VoidType, ArgTypes, false);
    this->aprof_increment_cost = Function::Create
            (AprofIncrementCostType, GlobalValue::ExternalLinkage, "aprof_increment_cost", pModule);

}

void AprofHook::InsertAprofInit(Instruction *firstInst) {
    CallInst *aprof_init_call = CallInst::Create(this->aprof_init, "", firstInst);
    aprof_init_call->setCallingConv(CallingConv::C);
    aprof_init_call->setTailCall(false);
    AttributeList int32_call_PAL;
    aprof_init_call->setAttributes(int32_call_PAL);

}

void AprofHook::InsertAProfIncrementCost(Instruction *BeforeInst) {
    CallInst *aprof_increment_cost_call = CallInst::Create(
            this->aprof_increment_cost, "", BeforeInst);
    aprof_increment_cost_call->setCallingConv(CallingConv::C);
    aprof_increment_cost_call->setTailCall(false);
    AttributeList void_call_PAL;
    aprof_increment_cost_call->setAttributes(void_call_PAL);
}

void AprofHook::SetupInit(Module *pModule) {
    // all set up operation
    SetupTypes(pModule);
    SetupConstants(pModule);
    SetupGlobals(pModule);
    SetupFunctions(pModule);
}

void AprofHook::SetupHooks(Module *pModule) {

    // init all global and constants variables
    SetupInit(pModule);

    Function *MainFunc = pModule->getFunction("main");
    if (MainFunc) {

        Instruction *firstInst = &*(MainFunc->begin()->begin());
        InsertAprofInit(firstInst);
    }

    for (Module::iterator FI = pModule->begin(); FI != pModule->end(); FI++) {

        Function *Func = &*FI;

        for (Function::iterator BI = Func->begin(); BI != Func->end(); BI++) {

            BasicBlock *BB = &*BI;
            Instruction *firstInst = &*(BB->begin());
            InsertAProfIncrementCost(firstInst);
        }
    }
}

bool AprofHook::runOnModule(Module &M) {
    SetupHooks(&M);
}

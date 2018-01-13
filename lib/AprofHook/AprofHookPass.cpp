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

void AprofHook::InsertAprofInit(Module *pModule, Instruction *firstInst) {
    std::vector<Type *> ArgTypes;
    FunctionType *AprofInitType = FunctionType::get(this->IntType, ArgTypes, false);
    this->aprof_init = Function::Create
            (AprofInitType, GlobalValue::ExternalLinkage, "aprof_init", pModule);

    CallInst *aprof_init_call =  CallInst::Create(this->aprof_init, "", firstInst);
    aprof_init_call->setCallingConv(CallingConv::C);
    aprof_init_call->setTailCall(false);
    AttributeList int32_call2_PAL;
    aprof_init_call->setAttributes(int32_call2_PAL);

}

void AprofHook::SetupInit(Module *pModule) {
    // all set up operation
    SetupTypes(pModule);
    SetupConstants(pModule);
    SetupGlobals(pModule);
}

void AprofHook::SetupHooks(Module *pModule) {

    // init all global and constants variables
    SetupInit(pModule);

    Function *MainFunc = pModule->getFunction("main");
    if (MainFunc) {

        Instruction *firstInst = &*(MainFunc->begin()->begin());
        InsertAprofInit(pModule, firstInst);
    }
}

bool AprofHook::runOnModule(Module &M) {
    SetupHooks(&M);
}

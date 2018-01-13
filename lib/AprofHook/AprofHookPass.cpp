#include "AprofHook/AprofHookPass.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DataLayout.h"
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

void AprofHook::SetupTypes() {

    this->VoidType = Type::getVoidTy(pModule->getContext());
    this->IntType = IntegerType::get(pModule->getContext(), 32);
    this->LongType = IntegerType::get(pModule->getContext(), 64);
    this->VoidPointerType = PointerType::get(
            IntegerType::get(pModule->getContext(), 8), 0);
}

void AprofHook::SetupConstants() {

    this->ConstantLong0 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("0"), 10));
    this->ConstantLong1 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("1"), 10));
}

void AprofHook::SetupGlobals() {

    assert(pModule->getGlobalVariable("aprof_count") == NULL);
    this->aprof_count = new GlobalVariable(
            *(this->pModule), this->LongType, false,
            GlobalValue::ExternalLinkage, 0,
            "aprof_count");
    this->aprof_count->setAlignment(8);
    this->aprof_count->setInitializer(this->ConstantLong0);

    assert(pModule->getGlobalVariable("aprof_bb_count") == NULL);
    this->aprof_bb_count = new GlobalVariable(
            *(this->pModule), this->LongType, false,
            GlobalValue::ExternalLinkage, 0,
            "aprof_bb_count");
    this->aprof_bb_count->setAlignment(8);
    this->aprof_bb_count->setInitializer(this->ConstantLong0);
}

void AprofHook::SetupFunctions() {

    std::vector<Type *> ArgTypes;

    // aprof_init
    FunctionType *AprofInitType = FunctionType::get(this->IntType, ArgTypes, false);
    this->aprof_init = Function::Create
            (AprofInitType, GlobalValue::ExternalLinkage, "aprof_init", this->pModule);
    ArgTypes.clear();

    // aprof_increment_cost
    FunctionType *AprofIncrementCostType = FunctionType::get(this->VoidType, ArgTypes, false);
    this->aprof_increment_cost = Function::Create
            (AprofIncrementCostType, GlobalValue::ExternalLinkage,
             "aprof_increment_cost", this->pModule);
    ArgTypes.clear();

    // aprof_write
    ArgTypes.push_back(this->VoidPointerType);
    ArgTypes.push_back(this->IntType);
    FunctionType *AprofWriteType = FunctionType::get(this->VoidType, ArgTypes, false);
    this->aprof_write = Function::Create
            (AprofWriteType, GlobalValue::ExternalLinkage, "aprof_write", this->pModule);
    this->aprof_write->setCallingConv(CallingConv::C);
    ArgTypes.clear();

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

void AprofHook::InsertAprofWrite(Value *var, Instruction *BeforeInst) {

    DataLayout *dl = new DataLayout(this->pModule);
    Type *type_1 = var->getType()->getContainedType(0);

    while (isa<PointerType>(type_1))
        type_1 = type_1->getContainedType(0);

    ConstantInt *const_int6 = ConstantInt::get(
            this->pModule->getContext(),
            APInt(32, StringRef(std::to_string(dl->getTypeAllocSize(type_1))), 10));

    CastInst* ptr_50 = new BitCastInst(var, this->VoidPointerType,
                                       "", BeforeInst);
    std::vector<Value*> void_51_params;
    void_51_params.push_back(ptr_50);
    void_51_params.push_back(const_int6);
    CallInst* void_51 = CallInst::Create(this->aprof_write, void_51_params, "", BeforeInst);
    void_51->setCallingConv(CallingConv::C);
    void_51->setTailCall(false);
    AttributeList void_PAL;
    void_51->setAttributes(void_PAL);

}

void AprofHook::SetupInit() {
    // all set up operation
    SetupTypes();
    SetupConstants();
    SetupGlobals();
    SetupFunctions();
}

void AprofHook::SetupHooks() {

    // init all global and constants variables
    SetupInit();

    Function *MainFunc = this->pModule->getFunction("main");
    if (MainFunc) {

        Instruction *firstInst = &*(MainFunc->begin()->begin());
        InsertAprofInit(firstInst);
    }

    for (Module::iterator FI = this->pModule->begin();
         FI != this->pModule->end(); FI++) {

        Function *Func = &*FI;

        for (Function::iterator BI = Func->begin(); BI != Func->end(); BI++) {

            BasicBlock *BB = &*BI;
            Instruction *firstInst = &*(BB->begin());
            if (firstInst) {
                InsertAProfIncrementCost(firstInst);
            }

            for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {

                Instruction *Inst = &*II;
                if (Inst->getOpcode() == Instruction::Store) {

                    Value *secondOp = Inst->getOperand(1);
                    Type *secondOpType = secondOp->getType();

                    while (isa<PointerType>(secondOpType)) {
                        secondOpType = secondOpType->getContainedType(0);
                    }

                    // ignore function pointer store!
                    if (!isa<FunctionType>(secondOpType)) {
                        InsertAprofWrite(secondOp, Inst);
                    }

                }

            }
        }

    }

}

bool AprofHook::runOnModule(Module &M) {
    this->pModule = &M;
    SetupHooks();
}

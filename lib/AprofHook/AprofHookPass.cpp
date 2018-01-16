#include "AprofHook/AprofHookPass.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"


using namespace llvm;
using namespace std;


static RegisterPass<AprofHook> X(
        "algo-profiling",
        "profiling algorithmic complexity",
        true, true);

/* */

int GetFunctionID(Function *F) {

    if (F->begin() == F->end()) {
        return -1;
    }

    Instruction *I = &*(F->begin()->begin());

    MDNode *Node = I->getMetadata("func_id");
    if (!Node) {
        return -1;
    }

    assert(Node->getNumOperands() == 1);
    const Metadata *MD = Node->getOperand(0);
    if (auto *MDV = dyn_cast<ValueAsMetadata>(MD)) {
        Value *V = MDV->getValue();
        ConstantInt *CI = dyn_cast<ConstantInt>(V);
        assert(CI);
        return CI->getZExtValue();
    }

    return -1;
}


int GetInstructionID(Instruction *II) {

    MDNode *Node = II->getMetadata("ins_id");
    if (!Node) {
        return -1;
    }

    assert(Node->getNumOperands() == 1);
    const Metadata *MD = Node->getOperand(0);
    if (auto *MDV = dyn_cast<ValueAsMetadata>(MD)) {
        Value *V = MDV->getValue();
        ConstantInt *CI = dyn_cast<ConstantInt>(V);
        assert(CI);
        return CI->getZExtValue();
    }

    return -1;
}

/* */

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

    // aprof_increment_rms
    FunctionType *AprofIncrementRmsType = FunctionType::get(this->VoidType, ArgTypes, false);
    this->aprof_increment_rms = Function::Create
            (AprofIncrementRmsType, GlobalValue::ExternalLinkage,
             "aprof_increment_rms", this->pModule);
    ArgTypes.clear();

    // aprof_write
    ArgTypes.push_back(this->VoidPointerType);
    ArgTypes.push_back(this->IntType);
    FunctionType *AprofWriteType = FunctionType::get(this->VoidType, ArgTypes, false);
    this->aprof_write = Function::Create
            (AprofWriteType, GlobalValue::ExternalLinkage, "aprof_write", this->pModule);
    this->aprof_write->setCallingConv(CallingConv::C);
    ArgTypes.clear();

    // aprof_read
    ArgTypes.push_back(this->VoidPointerType);
    ArgTypes.push_back(this->IntType);
    FunctionType *AprofReadType = FunctionType::get(this->VoidType, ArgTypes, false);
    this->aprof_read = Function::Create
            (AprofReadType, GlobalValue::ExternalLinkage, "aprof_read", this->pModule);
    this->aprof_read->setCallingConv(CallingConv::C);
    ArgTypes.clear();

    // aprof_call_before
    ArgTypes.push_back(this->IntType);
    ArgTypes.push_back(this->LongType);
    FunctionType *AprofCallBeforeType = FunctionType::get(this->VoidPointerType,
                                                          ArgTypes, false);
    this->aprof_call_before = Function::Create
            (AprofCallBeforeType, GlobalValue::ExternalLinkage, "aprof_call_before", this->pModule);
    this->aprof_call_before->setCallingConv(CallingConv::C);
    ArgTypes.clear();

    // aprof_return
    ArgTypes.push_back(this->LongType);
    FunctionType *AprofReturnType = FunctionType::get(this->VoidType, ArgTypes, false);
    this->aprof_return = Function::Create
            (AprofReturnType, GlobalValue::ExternalLinkage, "aprof_return", this->pModule);
    this->aprof_return->setCallingConv(CallingConv::C);
    ArgTypes.clear();

}

void AprofHook::InsertAprofInit(Instruction *firstInst) {

    CallInst *aprof_init_call = CallInst::Create(this->aprof_init, "", firstInst);
    aprof_init_call->setCallingConv(CallingConv::C);
    aprof_init_call->setTailCall(false);
    AttributeList int32_call_PAL;
    aprof_init_call->setAttributes(int32_call_PAL);

}

void AprofHook::InstrumentCostUpdater(BasicBlock *pBlock) {
    TerminatorInst *pTerminator = pBlock->getTerminator();
    LoadInst *pLoadnumCost = new LoadInst(this->aprof_bb_count, "", false, pTerminator);
    pLoadnumCost->setAlignment(8);
    BinaryOperator *pAdd = BinaryOperator::Create(
            Instruction::Add, pLoadnumCost, this->ConstantLong1, "add", pTerminator);

    StoreInst *pStore = new StoreInst(pAdd, this->aprof_bb_count, false, pTerminator);
    pStore->setAlignment(8);
}

void AprofHook::InsertAprofIncrementRms(Instruction *BeforeInst) {

    CallInst *aprof_increment_rms_call = CallInst::Create(
            this->aprof_increment_rms, "", BeforeInst);
    aprof_increment_rms_call->setCallingConv(CallingConv::C);
    aprof_increment_rms_call->setTailCall(false);
    AttributeList void_call_PAL;
    aprof_increment_rms_call->setAttributes(void_call_PAL);

}

void AprofHook::InsertAprofWrite(Value *var, Instruction *BeforeInst) {

    DataLayout *dl = new DataLayout(this->pModule);
    Type *type_1 = var->getType()->getContainedType(0);

    while (isa<PointerType>(type_1))
        type_1 = type_1->getContainedType(0);

    ConstantInt *const_int6 = ConstantInt::get(
            this->pModule->getContext(),
            APInt(32, StringRef(std::to_string(dl->getTypeAllocSize(type_1))), 10));

    CastInst *ptr_50 = new BitCastInst(var, this->VoidPointerType,
                                       "", BeforeInst);
    std::vector<Value *> void_51_params;
    void_51_params.push_back(ptr_50);
    void_51_params.push_back(const_int6);
    CallInst *void_51 = CallInst::Create(this->aprof_write, void_51_params, "", BeforeInst);
    void_51->setCallingConv(CallingConv::C);
    void_51->setTailCall(false);
    AttributeList void_PAL;
    void_51->setAttributes(void_PAL);

}

void AprofHook::InsertAprofRead(Value *var, Instruction *BeforeInst) {

    DataLayout *dl = new DataLayout(this->pModule);
    Type *type_1 = var->getType()->getContainedType(0);

    while (isa<PointerType>(type_1))
        type_1 = type_1->getContainedType(0);

    ConstantInt *const_int6 = ConstantInt::get(
            this->pModule->getContext(),
            APInt(32, StringRef(std::to_string(dl->getTypeAllocSize(type_1))), 10));

    CastInst *ptr_50 = new BitCastInst(var, this->VoidPointerType,
                                       "", BeforeInst);
    std::vector<Value *> void_51_params;
    void_51_params.push_back(ptr_50);
    void_51_params.push_back(const_int6);
    CallInst *void_51 = CallInst::Create(this->aprof_read, void_51_params, "", BeforeInst);
    void_51->setCallingConv(CallingConv::C);
    void_51->setTailCall(false);
    AttributeList void_PAL;
    void_51->setAttributes(void_PAL);

}

void AprofHook::InsertAprofAlloc(Value *var, Instruction *AfterInst) {

    DataLayout *dl = new DataLayout(this->pModule);
    Type *type_1 = var->getType();
    ConstantInt *const_int6 = ConstantInt::get(
            this->pModule->getContext(),
            APInt(32, StringRef(std::to_string(dl->getTypeAllocSize(type_1))), 10));

    CastInst *ptr_50 = new BitCastInst(var, this->VoidPointerType,
                                       "", AfterInst);
    std::vector<Value *> void_51_params;
    void_51_params.push_back(ptr_50);
    void_51_params.push_back(const_int6);
    CallInst *void_51 = CallInst::Create(this->aprof_read, void_51_params, "", AfterInst);
    void_51->setCallingConv(CallingConv::C);
    void_51->setTailCall(false);
    AttributeList void_PAL;
    void_51->setAttributes(void_PAL);

}


void AprofHook::InsertAprofCallBefore(int FuncID, Instruction *BeforeCallInst) {
    std::vector<Value *> vecParams;
    LoadInst *pLoad = new LoadInst(this->aprof_bb_count, "", false, BeforeCallInst);
    pLoad->setAlignment(8);

    ConstantInt *const_int6 = ConstantInt::get(
            this->pModule->getContext(),
            APInt(32, StringRef(std::to_string(FuncID)), 10));

    vecParams.push_back(const_int6);
    vecParams.push_back(pLoad);

    CallInst *void_46 = CallInst::Create(
            this->aprof_call_before, vecParams, "", BeforeCallInst);
    void_46->setCallingConv(CallingConv::C);
    void_46->setTailCall(false);
    AttributeList void_PAL;
    void_46->setAttributes(void_PAL);

}


void AprofHook::InsertAprofReturn(Instruction *BeforeInst) {
    std::vector<Value *> vecParams;
    LoadInst *pLoad = new LoadInst(this->aprof_bb_count, "", false, BeforeInst);
    pLoad->setAlignment(8);
    vecParams.push_back(pLoad);

    CallInst *void_49 = CallInst::Create(this->aprof_return, vecParams, "", BeforeInst);
    void_49->setCallingConv(CallingConv::C);
    void_49->setTailCall(false);
    AttributeList void_PAL;
    void_49->setAttributes(void_PAL);
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

    for (Module::iterator FI = this->pModule->begin();
         FI != this->pModule->end(); FI++) {

        Function *Func = &*FI;

        if (Func->hasInternalLinkage()) {
            continue;
        }

        int FuncID = GetFunctionID(Func);

        if (FuncID > 0) {
            errs() << FuncID << ":" << Func->getName() << ":" << Func->hasInternalLinkage() << "\n";
            Instruction *firstInst = &*(Func->begin()->begin());
            InsertAprofCallBefore(FuncID, firstInst);
        }

        for (Function::iterator BI = Func->begin(); BI != Func->end(); BI++) {

            BasicBlock *BB = &*BI;
            InstrumentCostUpdater(BB);

            for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {

                Instruction *Inst = &*II;

                if (GetInstructionID(Inst) == -1) {
                    continue;
                }

                switch (Inst->getOpcode()) {

                    case Instruction::Store: {
                        Value *secondOp = Inst->getOperand(1);
                        Type *secondOpType = secondOp->getType();

                        while (isa<PointerType>(secondOpType)) {
                            secondOpType = secondOpType->getContainedType(0);
                        }

                        // FIXME::ignore function pointer store!
                        if (!isa<FunctionType>(secondOpType)) {
                            InsertAprofWrite(secondOp, Inst);
                        }

                        break;
                    }

                    case Instruction::Alloca: {
                        II++;
                        Instruction *afterInst = &*II;
                        InsertAprofAlloc(Inst, afterInst);
                        II--;
                        break;
                    }

                    case Instruction::Load: {
                        // load instruction only has one operand !!!
                        Value *secondOp = Inst->getOperand(0);
                        InsertAprofRead(secondOp, Inst);

                        break;
                    }

                    case Instruction::Call: {
                        CallSite ci(Inst);
                        Function *Callee = dyn_cast<Function>(ci.getCalledValue()->stripPointerCasts());
                        // fgetc automatic rms++;
                        if (Callee->getName().str() == "fgetc") {
                            InsertAprofIncrementRms(Inst);
                        }
                        break;
                    }

                    case Instruction::Ret: {

                        InsertAprofReturn(Inst);

                        break;
                    }
                }
            }
        }
    }

    Function *MainFunc = this->pModule->getFunction("main");
    if (MainFunc) {

        Instruction *firstInst = &*(MainFunc->begin()->begin());
        InsertAprofInit(firstInst);
    }
}

bool AprofHook::runOnModule(Module &M) {

    this->pModule = &M;
    SetupHooks();

}

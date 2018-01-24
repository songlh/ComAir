#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include "AprofHook/AprofHookPass.h"
#include "Common/Constant.h"
#include "Common/Helper.h"


using namespace llvm;
using namespace std;


static RegisterPass<AprofHook> X(
        "algo-profiling",
        "profiling algorithmic complexity",
        true, true);

static cl::opt<int> onlyBBCount("only-bb-count",
                                cl::desc("only insert bb count."),
                                cl::init(0));

/* local function */

bool HasInsertFlag(Instruction *Inst, int flag) {

    int _flag = GetInstructionInsertFlag(Inst);

    if (_flag == flag) {
        return true;
    }

    return false;
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
//    ArgTypes.push_back(this->LongType);
    FunctionType *AprofCallBeforeType = FunctionType::get(this->VoidPointerType,
                                                          ArgTypes, false);
    this->aprof_call_before = Function::Create
            (AprofCallBeforeType, GlobalValue::ExternalLinkage, "aprof_call_before", this->pModule);
    this->aprof_call_before->setCallingConv(CallingConv::C);
    ArgTypes.clear();

    // aprof_return
    ArgTypes.push_back(this->LongType);
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

    int NumCost = GetBBCostNum(pBlock);

    if (NumCost > 0) {

        TerminatorInst *pTerminator = pBlock->getTerminator();
        LoadInst *pLoadnumCost = new LoadInst(this->BBAllocInst, "", true, pTerminator);
        pLoadnumCost->setAlignment(8);

        Type *tInt = Type::getInt64Ty(pModule->getContext());

        BinaryOperator *pAdd = BinaryOperator::Create(
                Instruction::Add, pLoadnumCost,
                ConstantInt::get(tInt, NumCost),
                "add", pTerminator);

        StoreInst *pStore = new StoreInst(pAdd, this->BBAllocInst, true, pTerminator);
        pStore->setAlignment(8);

    }
}

void AprofHook::InstrumentCostUpdater(Function *pFunction) {
    BasicBlock *pEntryBlock = &(pFunction->getEntryBlock());
    BasicBlock::iterator itCurrent = pEntryBlock->begin();
    Instruction *pInstBefore = &(*itCurrent);

    while (isa<AllocaInst>(pInstBefore)) {
        itCurrent++;
        pInstBefore = &(*itCurrent);
    }

    this->BBAllocInst = new AllocaInst(this->LongType, 0, "aprof_cost_bb", pInstBefore);
    this->BBAllocInst->setAlignment(8);
    StoreInst *pStore = new StoreInst(this->ConstantLong0, this->BBAllocInst, false, pInstBefore);
    pStore->setAlignment(8);

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

    ConstantInt *const_int6 = ConstantInt::get(
            this->pModule->getContext(),
            APInt(32, StringRef(std::to_string(FuncID)), 10));

    vecParams.push_back(const_int6);

    CallInst *void_46 = CallInst::Create(
            this->aprof_call_before, vecParams, "", BeforeCallInst);
    void_46->setCallingConv(CallingConv::C);
    void_46->setTailCall(false);
    AttributeList void_PAL;
    void_46->setAttributes(void_PAL);

}

void AprofHook::InstrumentRmsUpdater(Function *pFunction) {
    BasicBlock *pEntryBlock = &(pFunction->getEntryBlock());
    BasicBlock::iterator itCurrent = pEntryBlock->begin();
    Instruction *pInstBefore = &(*itCurrent);

    while (isa<AllocaInst>(pInstBefore)) {
        itCurrent++;
        pInstBefore = &(*itCurrent);
    }

    this->RmsAllocInst = new AllocaInst(this->LongType, 0, "aprof_rms", pInstBefore);
    this->RmsAllocInst->setAlignment(8);
    StoreInst *pStore = new StoreInst(this->ConstantLong0, this->RmsAllocInst, true, pInstBefore);
    pStore->setAlignment(8);

    for (Function::iterator BI = pFunction->begin(); BI != pFunction->end(); BI++) {

        BasicBlock *BB = &*BI;

        for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {
            Instruction *Inst = &*II;

            if (GetInstructionID(Inst) == -1) {
                continue;
            }

            switch (Inst->getOpcode()) {

                case Instruction::Call: {

                    CallSite ci(Inst);
                    Function *Callee = dyn_cast<Function>(ci.getCalledValue()->stripPointerCasts());

                    // fgetc automatic rms++;
                    if (Callee->getName().str() == "fgetc") {
                        LoadInst *pLoadnumCost = new LoadInst(this->RmsAllocInst, "", true, Inst);
                        pLoadnumCost->setAlignment(8);
                        BinaryOperator *pAdd = BinaryOperator::Create(
                                Instruction::Add, pLoadnumCost, this->ConstantLong1, "add", Inst);

                        StoreInst *pStore = new StoreInst(pAdd, this->RmsAllocInst, true, Inst);
                        pStore->setAlignment(8);
                    }

                    break;
                }
            }

        }
    }
}

void AprofHook::InsertAprofReturn(Instruction *BeforeInst, bool NeedUpdateRms) {
    std::vector<Value *> vecParams;
    LoadInst *bb_pLoad = new LoadInst(this->BBAllocInst, "", true, BeforeInst);
    bb_pLoad->setAlignment(8);
    vecParams.push_back(bb_pLoad);

    if (NeedUpdateRms) {
        LoadInst *rms_pLoad = new LoadInst(this->RmsAllocInst, "", true, BeforeInst);
        rms_pLoad->setAlignment(8);
        vecParams.push_back(rms_pLoad);
    } else {
        vecParams.push_back(this->ConstantLong0);
    }

    CallInst *void_49 = CallInst::Create(this->aprof_return, vecParams, "", BeforeInst);
    void_49->setCallingConv(CallingConv::C);
    void_49->setTailCall(false);
    AttributeList void_PAL;
    void_49->setAttributes(void_PAL);
}

void AprofHook::InsertAprofHooks(Function *Func) {

    int FuncID = GetFunctionID(Func);

    bool need_update_rms = false;

    if (FuncID > 0) {
        Instruction *firstInst = &*(Func->begin()->begin());
        InsertAprofCallBefore(FuncID, firstInst);
        InstrumentCostUpdater(Func);
//        InstrumentRmsUpdater(Func);
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
                    if (HasInsertFlag(Inst, WRITE)) {
                        Value *secondOp = Inst->getOperand(1);
                        Type *secondOpType = secondOp->getType();

                        while (isa<PointerType>(secondOpType)) {
                            secondOpType = secondOpType->getContainedType(0);
                        }
                        // FIXME::ignore function pointer store!
                        if (!isa<FunctionType>(secondOpType)) {
                            InsertAprofWrite(secondOp, Inst);
                        }
                    }

                    break;
                }

                case Instruction::Alloca: {
                    if (HasInsertFlag(Inst, READ)) {
                        II++;
                        Instruction *afterInst = &*II;
                        InsertAprofAlloc(Inst, afterInst);
                        II--;
                    }
                    break;
                }

                case Instruction::Load: {
                    // load instruction only has one operand !!!
                    if (HasInsertFlag(Inst, READ)) {
                        Value *firstOp = Inst->getOperand(0);
                        InsertAprofRead(firstOp, Inst);
                    }

                    break;
                }

                case Instruction::Call: {

                    CallSite ci(Inst);
                    Function *Callee = dyn_cast<Function>(ci.getCalledValue()->stripPointerCasts());

                    // fgetc automatic rms++;
                    if (Callee->getName().str() == "fgetc") {
                        need_update_rms = true;
                        InstrumentRmsUpdater(Inst->getFunction());
                    }

                    break;
                }

                case Instruction::Ret: {

                    InsertAprofReturn(Inst, need_update_rms);

                    break;
                }
            }
        }
    }

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

        if (IsIgnoreFunc(Func)) {
            continue;
        }

        InsertAprofHooks(Func);

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

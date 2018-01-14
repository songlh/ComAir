#include "AprofHook/AprofHookPass.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CallSite.h"
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

    // aprof_read
    ArgTypes.push_back(this->VoidPointerType);
    ArgTypes.push_back(this->IntType);
    FunctionType *AprofReadType = FunctionType::get(this->VoidType, ArgTypes, false);
    this->aprof_read = Function::Create
            (AprofReadType, GlobalValue::ExternalLinkage, "aprof_read", this->pModule);
    this->aprof_read->setCallingConv(CallingConv::C);
    ArgTypes.clear();

    // aprof_call_before
    PointerType *CharPointer = PointerType::get(
            IntegerType::get(pModule->getContext(), 8), 0);
    FunctionType *AprofCallBeforeType = FunctionType::get(this->VoidPointerType, CharPointer, false);
    this->aprof_call_before = Function::Create
            (AprofCallBeforeType, GlobalValue::ExternalLinkage, "aprof_call_before", this->pModule);
    this->aprof_call_before->setCallingConv(CallingConv::C);
    ArgTypes.clear();

    // aprof_call_after
    FunctionType *AprofCallAfterType = FunctionType::get(this->VoidType, ArgTypes, false);
    this->aprof_call_after = Function::Create
            (AprofCallAfterType, GlobalValue::ExternalLinkage, "aprof_call_after", this->pModule);
    this->aprof_call_after->setCallingConv(CallingConv::C);
    ArgTypes.clear();

    // aprof_return
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

void AprofHook::InsertAprofIncrementCost(Instruction *BeforeInst) {

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


void AprofHook::InsertAprofCallBefore(std::string FuncName, Instruction *BeforeCallInst) {

    Constant *const_string_funName = ConstantDataArray::getString(
            this->pModule->getContext(), FuncName, true);

    ArrayType *ArrayTy_FuncName = ArrayType::get(IntegerType::get(
            this->pModule->getContext(), 8), FuncName.size() + 1);

    GlobalVariable *gvar_array = new GlobalVariable(
            /*Module=*/*(this->pModule),
            /*Type=*/ArrayTy_FuncName,
            /*isConstant=*/true,
            /*Linkage=*/GlobalValue::PrivateLinkage,
            /*Initializer=*/0, // has initializer, specified below
            /*Name=*/FuncName);
    gvar_array->setAlignment(1);
    gvar_array->setInitializer(const_string_funName);

    ConstantInt *const_int32_26 = ConstantInt::get(
            this->pModule->getContext(), APInt(32, StringRef("0"), 10));

    std::vector<Constant *> const_ptr_30_indices;
    const_ptr_30_indices.push_back(const_int32_26);
    const_ptr_30_indices.push_back(const_int32_26);
    Constant *const_ptr_30 = ConstantExpr::getGetElementPtr(
            ArrayTy_FuncName, gvar_array, const_ptr_30_indices);

    CallInst *void_46 = CallInst::Create(
            this->aprof_call_before, const_ptr_30, "", BeforeCallInst);
    void_46->setCallingConv(CallingConv::C);
    void_46->setTailCall(false);
    AttributeList void_PAL;
    void_46->setAttributes(void_PAL);

}

void AprofHook::InsertAprofCallAfter(Instruction *AfterCallInst) {
    CallInst *void_49 = CallInst::Create(this->aprof_call_after, "", AfterCallInst);
    void_49->setCallingConv(CallingConv::C);
    void_49->setTailCall(false);
    AttributeList void_PAL;
    void_49->setAttributes(void_PAL);

}

void AprofHook::InsertAprofReturn(Instruction *BeforeInst) {
    CallInst *void_49 = CallInst::Create(this->aprof_return, "", BeforeInst);
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

    Function *MainFunc = this->pModule->getFunction("main");
    if (MainFunc) {

        Instruction *firstInst = &*(MainFunc->begin()->begin());
        InsertAprofInit(firstInst);
        InsertAprofCallBefore("main", firstInst);

        for (Function::iterator BI = MainFunc->begin(); BI != MainFunc->end(); BI++) {

            BasicBlock *BB = &*BI;

            for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {

                Instruction *Inst = &*II;

                if (Inst->getOpcode() == Instruction::Ret) {

                    // update main function cost
                    InsertAprofCallAfter(Inst);
                }
            }
        }
    }

    for (Module::iterator FI = this->pModule->begin();
         FI != this->pModule->end(); FI++) {

        Function *Func = &*FI;

        for (Function::iterator BI = Func->begin(); BI != Func->end(); BI++) {

            BasicBlock *BB = &*BI;
            Instruction *firstInst = &*(BB->begin());
            if (firstInst) {
                InsertAprofIncrementCost(firstInst);
            }

            for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {

                Instruction *Inst = &*II;

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

                    case Instruction::Call: {
                        CallSite cs(Inst);
                        Function *Callee = dyn_cast<Function>(cs.getCalledValue()->stripPointerCasts());

                        if (Callee) {
                            if (Callee->begin() != Callee->end()) {
                                InsertAprofCallBefore(Callee->getName(), Inst);
                                II++;
                                Instruction *AfterCallInst = &*II;
                                InsertAprofCallAfter(AfterCallInst);
                                II--;
                            }
                        }
                        break;
                    }

                    case Instruction::Load: {
                        // load instruction only has one operand !!!
                        Value *secondOp = Inst->getOperand(0);
                        Type *secondOpType = secondOp->getType();

                        while (isa<PointerType>(secondOpType)) {
                            secondOpType = secondOpType->getContainedType(0);
                        }

                        // FIXME::ignore function pointer store!
                        if (!isa<FunctionType>(secondOpType)) {
                            InsertAprofRead(secondOp, Inst);
                        }

                        break;
                    }

                    case Instruction::Ret: {
                        // FIXME:: If the function is main, it means Stack empty ?
                        InsertAprofReturn(Inst);
                        break;
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

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/raw_ostream.h"

#include "AprofHook/AprofHookPass.h"
#include "Common/Constant.h"
#include "Common/Helper.h"
#include "Support/BBProfiling.h"


using namespace llvm;
using namespace std;


static RegisterPass<AprofHook> X(
        "instrument-hooks",
        "profiling algorithmic complexity",
        false, false);


static cl::opt<int> onlyBBCount("only-bb-count",
                                cl::desc("only insert bb count."),
                                cl::init(0));


static cl::opt<int> isSampling("is-sampling",
                               cl::desc("Whether to perform sampling"),
                               cl::init(0));


static cl::opt<std::string> strFileName(
        "strFileName",
        cl::desc("The name of File to store system library"), cl::Optional,
        cl::value_desc("strFileName"));

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
    this->VoidPointerType = PointerType::get(IntegerType::get(pModule->getContext(), 8), 0);

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
    this->aprof_init = this->pModule->getFunction("aprof_init");
//    assert(this->aprof_init != NULL);

    if (!this->aprof_init) {
        FunctionType *AprofInitType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->aprof_init = Function::Create(AprofInitType, GlobalValue::ExternalLinkage, "aprof_init", this->pModule);
        ArgTypes.clear();
    }

    // aprof_increment_rms
    this->aprof_increment_rms = this->pModule->getFunction("aprof_increment_rms");
//    assert(this->aprof_increment_rms != NULL);

    if (!this->aprof_increment_rms) {
        ArgTypes.push_back(this->LongType);
        FunctionType *AprofIncrementRmsType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->aprof_increment_rms = Function::Create(AprofIncrementRmsType, GlobalValue::ExternalLinkage,
                                                     "aprof_increment_rms", this->pModule);
        ArgTypes.clear();
    }


    // aprof_write
    this->aprof_write = this->pModule->getFunction("aprof_write");
//    assert(this->aprof_write != NULL);

    if (!this->aprof_write) {
        ArgTypes.push_back(this->VoidPointerType);
        ArgTypes.push_back(this->IntType);
        FunctionType *AprofWriteType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->aprof_write = Function::Create(AprofWriteType, GlobalValue::ExternalLinkage, "aprof_write",
                                             this->pModule);
        this->aprof_write->setCallingConv(CallingConv::C);
        ArgTypes.clear();
    }

    // aprof_read
    this->aprof_read = this->pModule->getFunction("aprof_read");
//    assert(this->aprof_read != NULL);

    if (!this->aprof_read) {
        ArgTypes.push_back(this->VoidPointerType);
        ArgTypes.push_back(this->IntType);
        FunctionType *AprofReadType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->aprof_read = Function::Create(AprofReadType, GlobalValue::ExternalLinkage, "aprof_read", this->pModule);
        this->aprof_read->setCallingConv(CallingConv::C);
        ArgTypes.clear();
    }

    // aprof_call_before
    this->aprof_call_before = this->pModule->getFunction("aprof_call_before");
//    assert(this->aprof_call_before != NULL);

    if (!this->aprof_call_before) {
        ArgTypes.push_back(this->IntType);
        //ArgTypes.push_back(this->LongType);
        FunctionType *AprofCallBeforeType = FunctionType::get(this->VoidPointerType, ArgTypes, false);
        this->aprof_call_before = Function::Create(AprofCallBeforeType, GlobalValue::ExternalLinkage,
                                                   "aprof_call_before", this->pModule);
        this->aprof_call_before->setCallingConv(CallingConv::C);
        ArgTypes.clear();

    }

    // aprof_return
    this->aprof_return = this->pModule->getFunction("aprof_return");
//    assert(this->aprof_return != NULL);

    if (!this->aprof_return) {
        ArgTypes.push_back(this->LongType);
        //ArgTypes.push_back(this->LongType);
        FunctionType *AprofReturnType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->aprof_return = Function::Create(AprofReturnType, GlobalValue::ExternalLinkage, "aprof_return",
                                              this->pModule);
        this->aprof_return->setCallingConv(CallingConv::C);
        ArgTypes.clear();
    }

}

void AprofHook::InstrumentInit(Instruction *firstInst) {

    CallInst *aprof_init_call = CallInst::Create(this->aprof_init, "", firstInst);
    aprof_init_call->setCallingConv(CallingConv::C);
    aprof_init_call->setTailCall(false);
    AttributeList int32_call_PAL;
    aprof_init_call->setAttributes(int32_call_PAL);

}

void AprofHook::InstrumentCostUpdater(Function *pFunction) {

    BBProfilingGraph bbGraph = BBProfilingGraph(*pFunction);

    bbGraph.init();
    bbGraph.splitNotExitBlock();
    //bbGraph.printNodeEdgeInfo();

    //bbGraph.printNodeEdgeInfo();
    bbGraph.calculateSpanningTree();

    BBProfilingEdge *pQueryEdge = bbGraph.addQueryChord();
    bbGraph.calculateChordIncrements();

    Instruction *pInstBefore = pFunction->getEntryBlock().getFirstNonPHI();;


    this->BBAllocInst = new AllocaInst(this->LongType, 0, "numCost", pInstBefore);
    this->BBAllocInst->setAlignment(8);
    StoreInst *pStore = new StoreInst(this->ConstantLong0, this->BBAllocInst, false, pInstBefore);
    pStore->setAlignment(8);

    bbGraph.instrumentLocalCounterUpdate(this->BBAllocInst);

}

void AprofHook::InstrumentWrite(StoreInst *pStore, Instruction *BeforeInst) {

    Value *var = pStore->getOperand(1);

    DataLayout *dl = new DataLayout(this->pModule);
    Type *type_1 = var->getType()->getContainedType(0);

//    while (isa<PointerType>(type_1))
//        type_1 = type_1->getContainedType(0);

    if (type_1->isSized()) {

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

    } else {

        pStore->dump();
        type_1->dump();
        assert(false);

    }

}

void AprofHook::InstrumentRead(LoadInst *pLoad, Instruction *BeforeInst) {

    Value *var = pLoad->getOperand(0);

    DataLayout *dl = new DataLayout(this->pModule);
    Type *type_1 = var->getType()->getContainedType(0);

//    while (isa<PointerType>(type_1))
//        type_1 = type_1->getContainedType(0);

    if (type_1->isSized()) {
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

    } else {
        pLoad->dump();
        type_1->dump();
        assert(false);
    }
}

void AprofHook::InstrumentAlloc(Value *var, Instruction *BeforeInst) {

    DataLayout *dl = new DataLayout(this->pModule);
    Type *type_1 = var->getType();
    ConstantInt *const_int6 = ConstantInt::get(
            this->pModule->getContext(),
            APInt(32, StringRef(std::to_string(dl->getTypeAllocSize(type_1))), 10));

    CastInst *ptr_50 = new BitCastInst(var, this->VoidPointerType, "", BeforeInst);
    std::vector<Value *> void_51_params;
    void_51_params.push_back(ptr_50);
    void_51_params.push_back(const_int6);
    CallInst *void_51 = CallInst::Create(this->aprof_read, void_51_params, "", BeforeInst);
    void_51->setCallingConv(CallingConv::C);
    void_51->setTailCall(false);
    AttributeList void_PAL;
    void_51->setAttributes(void_PAL);

}

void AprofHook::InstrumentCallBefore(int FuncID, Instruction *BeforeCallInst) {
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


void AprofHook::InstrumentRmsUpdater(Function *F, Instruction *BeforeInst) {


    std::string funcName = F->getName();
    std::vector<Value *> vecParams;

    if (funcName == "fgetc") {

        vecParams.push_back(this->ConstantLong1);
        CallInst *void_49 = CallInst::Create(this->aprof_increment_rms,
                                             vecParams, "", BeforeInst);
        void_49->setCallingConv(CallingConv::C);
        void_49->setTailCall(false);
        AttributeList void_PAL;
        void_49->setAttributes(void_PAL);

    }

}


void AprofHook::ProcessMemIntrinsic(MemIntrinsic *memInst) {


    assert(memInst->getNumArgOperands() > 2);

    std::vector<Value *> vecParams;

    Value * lengthV = memInst->getArgOperand(2);

    if (ConstantInt *sizeCpy = dyn_cast<ConstantInt>(lengthV)) {
        vecParams.push_back(sizeCpy);
        CallInst *void_49 = CallInst::Create(this->aprof_increment_rms,
                                             vecParams, "", memInst);
        void_49->setCallingConv(CallingConv::C);
        void_49->setTailCall(false);
        AttributeList void_PAL;
        void_49->setAttributes(void_PAL);
    }

}


/*
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
    StoreInst *pStore = new StoreInst(this->ConstantLong0, this->RmsAllocInst, false, pInstBefore);
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
                        LoadInst *pLoadnumCost = new LoadInst(this->RmsAllocInst, "", false, Inst);
                        pLoadnumCost->setAlignment(8);
                        BinaryOperator *pAdd = BinaryOperator::Create(
                                Instruction::Add, pLoadnumCost, this->ConstantLong1, "add", Inst);

                        StoreInst *pStore = new StoreInst(pAdd, this->RmsAllocInst, false, Inst);
                        pStore->setAlignment(8);
                    }

                    break;
                }
            }
        }
    }
}
*/

void AprofHook::InstrumentReturn(Instruction *BeforeInst) {

    std::vector<Value *> vecParams;
    LoadInst *bb_pLoad = new LoadInst(this->BBAllocInst, "", false, BeforeInst);
    bb_pLoad->setAlignment(8);
    vecParams.push_back(bb_pLoad);

    CallInst *void_49 = CallInst::Create(this->aprof_return, vecParams, "", BeforeInst);
    void_49->setCallingConv(CallingConv::C);
    void_49->setTailCall(false);
    AttributeList void_PAL;
    void_49->setAttributes(void_PAL);
}

void AprofHook::InstrumentHooks(Function *Func) {

    int FuncID = GetFunctionID(Func);

    if (FuncID <= 0) {
        errs() << Func->getName() << "\n";
    }

    assert(FuncID > 0);

    //bool need_update_rms = false;

    if (FuncID > 0) {
        //Instruction *firstInst = &*(Func->begin()->begin());

        Instruction *firstInst = &*(Func->getEntryBlock().getFirstNonPHI());

        InstrumentCallBefore(FuncID, firstInst);
        InstrumentCostUpdater(Func);
    }

    for (Function::iterator BI = Func->begin(); BI != Func->end(); BI++) {

        BasicBlock *BB = &*BI;

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

                            if (StoreInst *pStore = dyn_cast<StoreInst>(Inst)) {
                                InstrumentWrite(pStore, Inst);
                            }
                        }
                    }

                    break;
                }

//                case Instruction::Alloca: {
//                    if (HasInsertFlag(Inst, READ)) {
//
//                        II++;
//                        Instruction *beforeInst = &*II;
//                        InstrumentAlloc(Inst, beforeInst);
//                        II--;
//
//                        ///InstrumentAlloc(Inst, beforeInst);
//                    }
//                    break;
//                }

                case Instruction::Load: {
                    // load instruction only has one operand !!!
                    if (HasInsertFlag(Inst, READ)) {
                        Value *firstOp = Inst->getOperand(0);
                        Type *firstOpType = firstOp->getType();

                        while (isa<PointerType>(firstOpType)) {
                            firstOpType = firstOpType->getContainedType(0);
                        }

                        if (!isa<FunctionType>(firstOpType)) {
                            if (LoadInst *pLoad = dyn_cast<LoadInst>(Inst)) {
                                InstrumentRead(pLoad, Inst);
                            }
                        }
                    }

                    break;
                }

                case Instruction::Call: {

                    if (isa<MemIntrinsic>(Inst)) {
                        ProcessMemIntrinsic(dyn_cast<MemIntrinsic>(Inst));
                        break;
                    }

                    CallSite ci(Inst);
                    Function *Callee = dyn_cast<Function>(ci.getCalledValue()->stripPointerCasts());

                    // fgetc automatic rms++;
                    if (Callee) {
                        InstrumentRmsUpdater(Callee, Inst);
                    }

                    break;
                }

                case Instruction::Ret: {
                    InstrumentReturn(Inst);
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

    for (Module::iterator FI = this->pModule->begin(); FI != this->pModule->end(); FI++) {
        Function *Func = &*FI;

        if (isSampling == 1) {
            if (!IsClonedFunc(Func)) {
                continue;
            }
        } else if (IsIgnoreFunc(Func)) {
            continue;
        }

        if (this->funNameIDFile)
            this->funNameIDFile << Func->getName().str()
                                << ":" << GetFunctionID(Func) << "\n";
        InstrumentHooks(Func);

    }

    Function *MainFunc = this->pModule->getFunction("main");
    assert(MainFunc != NULL);

    if (MainFunc) {

        //Instruction *firstInst = &*(MainFunc->begin()->begin());
        Instruction *firstInst = MainFunc->getEntryBlock().getFirstNonPHI();
        InstrumentInit(firstInst);
    }
}

bool AprofHook::runOnModule(Module &M) {

    if (strFileName.empty()) {

        errs() << "You not set outfile to save function name and ID." << "\n";
        return false;

    } else {

        this->funNameIDFile.open(strFileName, std::ofstream::out | std::ofstream::app);
    }

    this->pModule = &M;
    SetupHooks();

    return true;
}

#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/DebugInfo.h"

#include "RecursiveProfiling/RecursiveRmsHook/RecursiveRmsHook.h"
#include "Common/Constant.h"
#include "Common/Helper.h"
#include "Support/BBProfiling.h"


using namespace llvm;
using namespace std;


static RegisterPass<RecursiveRmsHook> X(
        "interested-recursive-hook",
        "only instrument hook in interested recursive function call",
        false, false);


static cl::opt<std::string> strFileName(
        "strFileName",
        cl::desc("The name of File to store system library"), cl::Optional,
        cl::value_desc("strFileName"));


static cl::opt<int> isSampling("is-sampling",
                               cl::desc("Whether to perform sampling"),
                               cl::init(0));


static cl::opt<std::string> strFuncName(
        "strFuncName",
        cl::desc("The name of function to instrumention."), cl::Optional,
        cl::value_desc("strFuncName"));


/* local function */

bool hasCallReturn(BasicBlock *BB) {

    for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {
        Instruction *Inst = &*II;

        if (Inst->getOpcode() == Instruction::Call) {
            CallSite ci(Inst);
            Function *Callee = dyn_cast<Function>(
                    ci.getCalledValue()->stripPointerCasts());

            if (Callee->getName() == "aprof_return") {
                return true;
            }
        }
    }

    return false;
}

/* */

char RecursiveRmsHook::ID = 0;

void RecursiveRmsHook::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<ScalarEvolutionWrapperPass>();
}

RecursiveRmsHook::RecursiveRmsHook() : ModulePass(ID) {
    PassRegistry &Registry = *PassRegistry::getPassRegistry();
    initializeScalarEvolutionWrapperPassPass(Registry);
}

void RecursiveRmsHook::SetupTypes() {

    this->VoidType = Type::getVoidTy(pModule->getContext());
    this->IntType = IntegerType::get(pModule->getContext(), 32);
    this->LongType = IntegerType::get(pModule->getContext(), 64);
    this->VoidPointerType = PointerType::get(IntegerType::get(pModule->getContext(), 8), 0);

}

void RecursiveRmsHook::SetupConstants() {

    this->ConstantLong0 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("0"), 10));
    this->ConstantLong1 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("1"), 10));
    this->ConstantLongN1 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("-1"), 10));

}

void RecursiveRmsHook::SetupGlobals() {

    assert(pModule->getGlobalVariable("numCost") == NULL);
    this->numCost = new GlobalVariable(*pModule, this->LongType, false, GlobalValue::ExternalLinkage, 0, "numCost");
    this->numCost->setAlignment(8);
    this->numCost->setInitializer(this->ConstantLong0);

}

void RecursiveRmsHook::SetupFunctions() {

    std::vector<Type *> ArgTypes;

    // aprof_init
    this->aprof_init = this->pModule->getFunction("aprof_init");
//    assert(this->aprof_init != NULL);

    if (!this->aprof_init) {
        FunctionType *AprofInitType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->aprof_init = Function::Create(AprofInitType, GlobalValue::ExternalLinkage, "aprof_init", this->pModule);
        ArgTypes.clear();
    }

    // aprof_call_before
    this->aprof_call_before = this->pModule->getFunction("aprof_call_before");
//    assert(this->aprof_call_before != NULL);

    if (!this->aprof_call_before) {
        ArgTypes.push_back(this->IntType);
        //ArgTypes.push_back(this->LongType);
        FunctionType *AprofCallBeforeType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->aprof_call_before = Function::Create(AprofCallBeforeType, GlobalValue::ExternalLinkage,
                                                   "aprof_call_before", this->pModule);
        this->aprof_call_before->setCallingConv(CallingConv::C);
        ArgTypes.clear();

    }

    // aprof_read
    this->aprof_read = this->pModule->getFunction("aprof_read");
//    assert(this->aprof_read != NULL);

    if (!this->aprof_read) {
        ArgTypes.push_back(this->VoidPointerType);
        ArgTypes.push_back(this->LongType);
//        ArgTypes.push_back(this->IntType);
        FunctionType *AprofReadType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->aprof_read = Function::Create(AprofReadType, GlobalValue::ExternalLinkage, "aprof_read", this->pModule);
        this->aprof_read->setCallingConv(CallingConv::C);
        ArgTypes.clear();
    }

    // aprof_write
    this->aprof_write = this->pModule->getFunction("aprof_write");
//    assert(this->aprof_write != NULL);

    if (!this->aprof_write) {
        ArgTypes.push_back(this->VoidPointerType);
        ArgTypes.push_back(this->LongType);
        FunctionType *AprofWriteType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->aprof_write = Function::Create(AprofWriteType, GlobalValue::ExternalLinkage, "aprof_write",
                                             this->pModule);
        this->aprof_write->setCallingConv(CallingConv::C);
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

    // aprof_return(int funcID, unsigned long callStack, unsigned long numCost)
    this->aprof_return = this->pModule->getFunction("aprof_return");
//    assert(this->aprof_return != NULL);

    if (!this->aprof_return) {
        ArgTypes.push_back(this->IntType);
        ArgTypes.push_back(this->LongType);
        FunctionType *AprofReturnType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->aprof_return = Function::Create(AprofReturnType, GlobalValue::ExternalLinkage, "aprof_return",
                                              this->pModule);
        this->aprof_return->setCallingConv(CallingConv::C);
        ArgTypes.clear();
    }

    // aprof_final
    this->aprof_final = this->pModule->getFunction("aprof_final");
//    assert(this->aprof_return != NULL);

    if (!this->aprof_final) {
        //ArgTypes.push_back(this->LongType);
        FunctionType *AprofReturnType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->aprof_final = Function::Create(AprofReturnType, GlobalValue::ExternalLinkage, "aprof_final",
                                             this->pModule);
        this->aprof_final->setCallingConv(CallingConv::C);
        ArgTypes.clear();
    }

}

void RecursiveRmsHook::InstrumentWrite(StoreInst *pStore, Instruction *BeforeInst) {

    Value *var = pStore->getOperand(1);

    DataLayout *dl = new DataLayout(this->pModule);
    Type *type_1 = var->getType()->getContainedType(0);

//    while (isa<PointerType>(type_1))
//        type_1 = type_1->getContainedType(0);

    if (type_1->isSized()) {

        ConstantInt *const_int6 = ConstantInt::get(
                this->pModule->getContext(),
                APInt(64, StringRef(std::to_string(dl->getTypeAllocSize(type_1))), 10));

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

void RecursiveRmsHook::InstrumentInit(Instruction *firstInst) {

    CallInst *aprof_init_call = CallInst::Create(this->aprof_init, "", firstInst);
    aprof_init_call->setCallingConv(CallingConv::C);
    aprof_init_call->setTailCall(false);
    AttributeList int32_call_PAL;
    aprof_init_call->setAttributes(int32_call_PAL);

}

void RecursiveRmsHook::InstrumentCallBefore(Function *F) {

    Instruction *FirstInst = &*(F->getEntryBlock().getFirstInsertionPt());

    int FuncID = GetFunctionID(F);

    ConstantInt *const_funId = ConstantInt::get(
            this->pModule->getContext(),
            APInt(32, StringRef(std::to_string(FuncID)), 10));

    // call aprof_call_before
    std::vector<Value *> vecParams;
    vecParams.push_back(const_funId);
    CallInst *void_49 = CallInst::Create(this->aprof_call_before, vecParams, "", FirstInst);
    void_49->setCallingConv(CallingConv::C);
    void_49->setTailCall(false);
    AttributeList void_PAL;
    void_49->setAttributes(void_PAL);

}

void RecursiveRmsHook::InstrumentReturn(Instruction *BeforeInst) {

    int FuncID = GetFunctionID(BeforeInst->getFunction());

    ConstantInt *const_funId = ConstantInt::get(
            this->pModule->getContext(),
            APInt(32, StringRef(std::to_string(FuncID)), 10));

    // call_stack updater
    LoadInst *pLoadnumCost = new LoadInst(this->numCost, "", false, 8, BeforeInst);

    // call aprof_return
    std::vector<Value *> vecParams;
    vecParams.push_back(const_funId);
    vecParams.push_back(pLoadnumCost);
    CallInst *void_49 = CallInst::Create(this->aprof_return, vecParams, "", BeforeInst);
    void_49->setCallingConv(CallingConv::C);
    void_49->setTailCall(false);
    AttributeList void_PAL;
    void_49->setAttributes(void_PAL);

}

void RecursiveRmsHook::InstrumentRead(LoadInst *pLoad, Instruction *BeforeInst) {

    Value *var = pLoad->getOperand(0);

    if (var->getName().str() == "numCost") {
        return;
    }

    DataLayout *dl = new DataLayout(this->pModule);
    Type *type_1 = var->getType()->getContainedType(0);

    if (type_1->isSized()) {
        ConstantInt *const_int6 = ConstantInt::get(
                this->pModule->getContext(),
                APInt(64, StringRef(std::to_string(dl->getTypeAllocSize(type_1))), 10));

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

void RecursiveRmsHook::InstrumentHooks(Function *Func) {

    int FuncID = GetFunctionID(Func);

    if (FuncID <= 0) {
        errs() << Func->getName() << "\n";
    }

    assert(FuncID > 0);

    // be carefull, there must be this order!
    InstrumentCallBefore(Func);
    InstrumentRmsUpdater(Func);
    InstrumentUpdater(Func);

    for (Function::iterator BI = Func->begin(); BI != Func->end(); BI++) {

        BasicBlock *BB = &*BI;

        for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {
            Instruction *Inst = &*II;

            if (GetInstructionID(Inst) == -1) {
                continue;
            }

            switch (Inst->getOpcode()) {

                case Instruction::Load: {
                    if (LoadInst *pLoad = dyn_cast<LoadInst>(Inst)) {
                        InstrumentRead(pLoad, Inst);
                    }
                    break;
                }

                case Instruction::Store: {
                    if (StoreInst *pStore = dyn_cast<StoreInst>(Inst)) {
                        InstrumentWrite(pStore, Inst);
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


void RecursiveRmsHook::InstrumentFinal(Function *mainFunc) {

    for (Function::iterator BI = mainFunc->begin(); BI != mainFunc->end(); BI++) {

        BasicBlock *BB = &*BI;

        for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {

            Instruction *Inst = &*II;

            switch (Inst->getOpcode()) {
                case Instruction::Ret: {
                    std::vector<Value *> vecParams;
                    CallInst *void_49 = CallInst::Create(this->aprof_final, vecParams, "", Inst);
                    void_49->setCallingConv(CallingConv::C);
                    void_49->setTailCall(false);
                    AttributeList void_PAL;
                    void_49->setAttributes(void_PAL);
                }
            }
        }
    }

}

void RecursiveRmsHook::InstrumentRmsUpdater(Function *F) {

    if (F->arg_size() > 0) {
        DataLayout *dl = new DataLayout(this->pModule);
        int add_rms = 0;
        for (Function::arg_iterator argIt = F->arg_begin(); argIt != F->arg_end(); argIt++) {
            Argument *argument = argIt;

            Type *argType = argument->getType();
            add_rms += dl->getTypeAllocSize(argType);
        }

        Instruction *II = &*(F->getEntryBlock().getFirstInsertionPt());

        AllocaInst *temAllocInst = new AllocaInst(this->LongType, 0, "tempAlloc", II);
        CastInst *ptr_50 = new BitCastInst(temAllocInst, this->VoidPointerType,
                                           "");
        ptr_50->insertAfter(II);

        std::vector<Value *> vecParams;
        ConstantInt *const_rms = ConstantInt::get(
                this->pModule->getContext(),
                APInt(64, StringRef(std::to_string(add_rms)), 10));
        vecParams.push_back(ptr_50);
        vecParams.push_back(const_rms);
        CallInst *void_49 = CallInst::Create(this->aprof_read,
                                             vecParams, "");
        void_49->insertAfter(ptr_50);
        void_49->setCallingConv(CallingConv::C);
        void_49->setTailCall(false);
        AttributeList void_PAL;
        void_49->setAttributes(void_PAL);
    }

}

void RecursiveRmsHook::InstrumentUpdater(Function *pFunction) {

    Instruction *FirstInst = &*(pFunction->getEntryBlock().getFirstInsertionPt());
    LoadInst *pLoadnumCost = new LoadInst(this->numCost, "", false, 8, FirstInst);
    BinaryOperator *pAdd = BinaryOperator::Create(Instruction::Add, pLoadnumCost,
                                                  this->ConstantLong1, "add",
                                                  FirstInst);
    new StoreInst(pAdd, this->numCost, false, 8, FirstInst);

}

void RecursiveRmsHook::SetupInit() {
    // all set up operation
    SetupTypes();
    SetupConstants();
    SetupGlobals();
    SetupFunctions();

}

void RecursiveRmsHook::SetupHooks() {

    if (!strFuncName.empty()) {
        Function *Func = this->pModule->getFunction(strFuncName);

        if (!Func) {
            errs() << "could not find target function." << "\n";
            exit(0);
        }

    }

    for (Module::iterator FI = this->pModule->begin(); FI != this->pModule->end(); FI++) {

        Function *Func = &*FI;

        if (!IsRecursiveCall(Func)) {
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
        Instruction *firstInst = &*MainFunc->getEntryBlock().getFirstInsertionPt();
        InstrumentInit(firstInst);
        InstrumentFinal(MainFunc);

        // this is make sure main function has call before,
        // so the shadow stack will not empty in running.
    }
}

void RecursiveRmsHook::SetupSampleRecursiveHooks() {


    if (strFuncName.empty()) {
        errs() << "Please set recursive function name." << "\n";
        exit(0);
    }

    Function *Func = this->pModule->getFunction(strFuncName);

    if (!Func || !IsRecursiveCall(Func)) {
        errs() << "could not find target recursive function." << "\n";
        exit(0);
    }

    string new_name = Func->getName().str();
    new_name = CLONE_FUNCTION_PREFIX + new_name;

    Function *NewFunc = this->pModule->getFunction(new_name);

    if (!NewFunc) {
        errs() << "could not find cloned recursive function." << "\n";
        exit(0);
    }


    if (this->funNameIDFile)
        this->funNameIDFile << strFuncName
                            << ":" << GetFunctionID(Func) << "\n";

    InstrumentHooks(NewFunc);


    Function *MainFunc = this->pModule->getFunction("main");
    assert(MainFunc != NULL);

    if (MainFunc) {

        //Instruction *firstInst = &*(MainFunc->begin()->begin());
        Instruction *firstInst = &*MainFunc->getEntryBlock().getFirstInsertionPt();
        InstrumentInit(firstInst);
        InstrumentFinal(MainFunc);

        // this is make sure main function has call before,
        // so the shadow stack will not empty in running.
    }
}

bool RecursiveRmsHook::hasRecursiveCall(Module &M) {

    bool flag = false;

    for (Module::iterator FI = M.begin(); FI != M.end(); FI++) {
        Function *Func = &*FI;

        if (IsRecursiveCall(Func)) {
            errs() << Func->getName() << "\n";
            flag = true;
        }
    }

    return flag;
}

bool RecursiveRmsHook::runOnModule(Module &M) {

    if (!hasRecursiveCall(M)) {
        errs() << "Current module has no recursive call,"
                " please set a right bc file!" << "\n";
        return false;
    }

    if (strFileName.empty()) {

        errs() << "You not set outfile to save function name and ID." << "\n";
        return false;

    } else {

        this->funNameIDFile.open(strFileName, std::ofstream::out);
    }

    this->pModule = &M;

    SetupInit();

    if (isSampling) {

        SetupSampleRecursiveHooks();

    } else {

        SetupHooks();
    }

    return true;
}

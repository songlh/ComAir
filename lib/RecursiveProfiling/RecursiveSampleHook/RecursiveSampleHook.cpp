#include <string>

#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/DebugInfo.h"

#include "RecursiveProfiling/RecursiveSampleHook/RecursiveSampleHook.h"
#include "Common/Constant.h"
#include "Common/Helper.h"
#include "Support/BBProfiling.h"


using namespace llvm;
using namespace std;


static RegisterPass<RecursiveSampleHook> X(
        "recursive-sample-hook",
        "only instrument hook in interested recursive function call",
        false, false);


static cl::opt<std::string> strFileName(
        "strFileName",
        cl::desc("The name of File to store system library"), cl::Optional,
        cl::value_desc("strFileName"));


static cl::opt<int> SamplingRate("sampleRate",
                                 cl::desc("The rate of sampling."),
                                 cl::init(100));


static cl::opt<std::string> strFuncName(
        "strFuncName",
        cl::desc("The name of function to instrumention."), cl::Optional,
        cl::value_desc("strFuncName"));


char RecursiveSampleHook::ID = 0;

void RecursiveSampleHook::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
}

RecursiveSampleHook::RecursiveSampleHook() : ModulePass(ID) {
    PassRegistry &Registry = *PassRegistry::getPassRegistry();
}

void RecursiveSampleHook::SetupTypes() {

    this->VoidType = Type::getVoidTy(pModule->getContext());
    this->IntType = IntegerType::get(pModule->getContext(), 32);
    this->LongType = IntegerType::get(pModule->getContext(), 64);
    this->VoidPointerType = PointerType::get(IntegerType::get(pModule->getContext(), 8), 0);

}

void RecursiveSampleHook::SetupConstants() {

    // long
    this->ConstantLong0 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("0"), 10));
    this->ConstantLong1 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("1"), 10));

    // int
    this->ConstantInt0 = ConstantInt::get(pModule->getContext(), APInt(32, StringRef("0"), 10));
    this->ConstantInt1 = ConstantInt::get(pModule->getContext(), APInt(32, StringRef("1"), 10));
    this->ConstantInt2 = ConstantInt::get(pModule->getContext(), APInt(32, StringRef("2"), 10));
    this->ConstantInt3 = ConstantInt::get(pModule->getContext(), APInt(32, StringRef("3"), 10));
//    this->ConstantSamplingRate = ConstantInt::get(pModule->getContext(),
//                                                  APInt(32, StringRef(std::to_string(SamplingRate)), 10));

}

void RecursiveSampleHook::SetupGlobals() {
    assert(pModule->getGlobalVariable("numCost") == NULL);
    this->numCost = new GlobalVariable(*pModule, this->LongType, false, GlobalValue::ExternalLinkage, 0, "numCost");
    this->numCost->setAlignment(8);
    this->numCost->setInitializer(this->ConstantLong0);

//    assert(pModule->getGlobalVariable("Switcher") != NULL);
//    this->Switcher = this->pModule->getGlobalVariable("Switcher");

    assert(pModule->getGlobalVariable("SampleMonitor") == NULL);
    this->SampleMonitor = new GlobalVariable(*pModule, this->IntType,
                                             false, GlobalValue::ExternalLinkage,
                                             0, "SampleMonitor");
    this->SampleMonitor->setAlignment(4);
    this->SampleMonitor->setInitializer(this->ConstantInt0);

}

void RecursiveSampleHook::SetupFunctions() {

    std::vector<Type *> ArgTypes;

    // aprof_init
    this->aprof_init = this->pModule->getFunction("aprof_init");
//    assert(this->aprof_init != NULL);

    if (!this->aprof_init) {
        FunctionType *AprofInitType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->aprof_init = Function::Create(AprofInitType, GlobalValue::ExternalLinkage, "aprof_init", this->pModule);
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
//        ArgTypes.push_back(this->IntType);
        FunctionType *AprofWriteType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->aprof_write = Function::Create(AprofWriteType, GlobalValue::ExternalLinkage, "aprof_write",
                                             this->pModule);
        this->aprof_write->setCallingConv(CallingConv::C);
        ArgTypes.clear();
    }

    // aprof_return
    this->aprof_return = this->pModule->getFunction("aprof_return");
//    assert(this->aprof_return != NULL);

    if (!this->aprof_return) {
        ArgTypes.push_back(this->LongType);
        ArgTypes.push_back(this->IntType);
        FunctionType *AprofReturnType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->aprof_return = Function::Create(AprofReturnType, GlobalValue::ExternalLinkage, "aprof_return",
                                              this->pModule);
        this->aprof_return->setCallingConv(CallingConv::C);
        ArgTypes.clear();
    }

    this->aprof_geo = this->pModule->getFunction("aprof_geo");
    if (!this->aprof_geo) {
        // aprof_geo
        ArgTypes.push_back(this->IntType);
        FunctionType *AprofGeoType = FunctionType::get(this->IntType, ArgTypes, false);
        this->aprof_geo = Function::Create
                (AprofGeoType, GlobalValue::ExternalLinkage, "aprof_geo", this->pModule);
        this->aprof_geo->setCallingConv(CallingConv::C);
        ArgTypes.clear();
    }

}

void RecursiveSampleHook::InstrumentWrite(StoreInst *pStore, Instruction *BeforeInst) {

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

void RecursiveSampleHook::InstrumentInit(Instruction *firstInst) {

    CallInst *aprof_init_call = CallInst::Create(this->aprof_init, "", firstInst);
    aprof_init_call->setCallingConv(CallingConv::C);
    aprof_init_call->setTailCall(false);
    AttributeList int32_call_PAL;
    aprof_init_call->setAttributes(int32_call_PAL);

}

void RecursiveSampleHook::InstrumentReturn(Instruction *BeforeInst) {

    std::vector<Value *> vecParams;
    LoadInst *bb_pLoad = new LoadInst(this->numCost, "", false, 8, BeforeInst);
    LoadInst *pLoad = new LoadInst(this->SampleMonitor, "", false, 4, BeforeInst);
    vecParams.push_back(bb_pLoad);
    vecParams.push_back(pLoad);
    CallInst *void_49 = CallInst::Create(this->aprof_return, vecParams, "", BeforeInst);
    void_49->setCallingConv(CallingConv::C);
    void_49->setTailCall(false);
    AttributeList void_PAL;
    void_49->setAttributes(void_PAL);

}

void RecursiveSampleHook::InstrumentRead(LoadInst *pLoad, Instruction *BeforeInst) {

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

void  RecursiveSampleHook::InstrumentRmsUpdater(Function *F) {

    if (F->arg_size() > 0) {

        DataLayout *dl = new DataLayout(this->pModule);
        int add_rms = 0;
        for (Function::arg_iterator argIt = F->arg_begin(); argIt != F->arg_end(); argIt++) {
            Argument *argument = argIt;

            Type *argType = argument->getType();
            add_rms += dl->getTypeAllocSize(argType);
        }

        Instruction *II = &*(F->getEntryBlock().getFirstInsertionPt());

        AllocaInst *tempAlloc = new AllocaInst(this->LongType, 0, "tempLocal", II);
        CastInst *ptr_50 = new BitCastInst(tempAlloc, this->VoidPointerType, "");
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

void RecursiveSampleHook::InstrumentOriginalReturn(Function *Func) {
    for (Function::iterator BI = Func->begin(); BI != Func->end(); BI++) {

        BasicBlock *BB = &*BI;

        for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {
            Instruction *Inst = &*II;
            switch (Inst->getOpcode()) {
                case Instruction::Ret: {
                    InstrumentReturn(Inst);
                    break;
                }
            }
        }
    }
}

void RecursiveSampleHook::InstrumentHooks(Function *Func) {

    for (Function::iterator BI = Func->begin(); BI != Func->end(); BI++) {

        BasicBlock *BB = &*BI;

        for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {
            Instruction *Inst = &*II;

            if (GetInstructionID(Inst) == -1) {
                continue;
            }

            switch (Inst->getOpcode()) {

//                case Instruction::Load: {
//                    if (LoadInst *pLoad = dyn_cast<LoadInst>(Inst)) {
//                        InstrumentRead(pLoad, Inst);
//                    }
//                    break;
//                }

//                case Instruction::Store: {
//                    if (StoreInst *pStore = dyn_cast<StoreInst>(Inst)) {
//                        InstrumentWrite(pStore, Inst);
//                    }
//                    break;
//                }

                case Instruction::Ret: {
                    InstrumentReturn(Inst);
                    break;
                }
            }
        }
    }
}


void RecursiveSampleHook::InstrumentCostUpdater(Function *pFunction) {

    Instruction *FirstInst = &*(pFunction->getEntryBlock().getFirstInsertionPt());
    LoadInst *pLoadnumCost = new LoadInst(this->numCost, "", false, 8, FirstInst);
    BinaryOperator *pAdd = BinaryOperator::Create(Instruction::Add, pLoadnumCost,
                                                  this->ConstantLong1, "add",
                                                  FirstInst);
    new StoreInst(pAdd, this->numCost, false, 8, FirstInst);

}

void RecursiveSampleHook::SetupInit() {
    // all set up operation
    SetupTypes();
    SetupConstants();
    SetupGlobals();
    SetupFunctions();

}

void RecursiveSampleHook::SetupHooks() {

    if (strFuncName.empty()) {
        errs() << "Please set recursive function name." << "\n";
        exit(0);
    }

    Function *Func = this->pModule->getFunction(strFuncName);

    if (!Func) {
        errs() << "could not find target recursive function." << "\n";
        exit(0);
    }

    std::string new_name = getClonedFunctionName(this->pModule, strFuncName);
    Function *NewFunc = this->pModule->getFunction(new_name);

    if (!NewFunc) {
        errs() << "could not find cloned recursive function." << "\n";
        exit(0);
    }


    if (this->funNameIDFile)
        this->funNameIDFile << strFuncName
                            << ":" << GetFunctionID(Func) << "\n";

    InstrumentCostUpdater(Func);
//    InstrumentOriginalReturn(Func);
    InstrumentRmsUpdater(NewFunc);
    InstrumentCostUpdater(NewFunc);
    InstrumentHooks(NewFunc);


    Function *MainFunc = this->pModule->getFunction("main");
    assert(MainFunc != NULL);

    if (MainFunc) {

        //Instruction *firstInst = &*(MainFunc->begin()->begin());
        Instruction *firstInst = &*MainFunc->getEntryBlock().getFirstInsertionPt();
        InstrumentInit(firstInst);
        InstrumentOriginalReturn(MainFunc);

        // this is make sure main function has call before,
        // so the shadow stack will not empty in running.

    }
}

bool RecursiveSampleHook::hasRecursiveCall(Module &M) {

    bool flag = false;

    for (Module::iterator FI = M.begin(); FI != M.end(); FI++) {
        Function *Func = &*FI;

        if (IsRecursiveCall(Func)) {
//            errs() << Func->getName() << "\n";
            flag = true;
        }
    }

    return flag;
}

bool RecursiveSampleHook::runOnModule(Module &M) {

//    if (!hasRecursiveCall(M)) {
//        errs() << "Current module has no recursive call,"
//                " please set a right bc file!" << "\n";
//        return false;
//    }

    if (strFileName.empty()) {

        errs() << "You not set outfile to save function name and ID." << "\n";
        return false;

    } else {

        this->funNameIDFile.open(strFileName, std::ofstream::out);
    }

    this->pModule = &M;

    SetupInit();

    SetupHooks();

    return true;
}

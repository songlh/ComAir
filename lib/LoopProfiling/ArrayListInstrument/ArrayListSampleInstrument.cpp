#include <map>
#include <set>
#include <vector>

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"


#include "LoopProfiling/ArrayListInstrument/ArrayListInstrument.h"
#include "Common/ArrayLisnkedIndentifier.h"

using namespace llvm;
using namespace std;


static RegisterPass<ArrayListInstrument> X("array-list-instrument",
                                           "instrument a loop accessing an array element in each iteration",
                                           true, false);

static cl::opt<unsigned> uSrcLine("noLine",
                                  cl::desc("Source Code Line Number for the Loop"), cl::Optional,
                                  cl::value_desc("uSrcCodeLine"));

static cl::opt<std::string> strFileName("strFile",
                                        cl::desc("File Name for the Loop"), cl::Optional,
                                        cl::value_desc("strFileName"));

static cl::opt<std::string> strFuncName("strFunc",
                                        cl::desc("Function Name"), cl::Optional,
                                        cl::value_desc("strFuncName"));


char ArrayListInstrument::ID = 0;



void ArrayListInstrument::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesCFG();
    AU.addRequired<LoopInfoWrapperPass>();
}

ArrayListInstrument::ArrayListInstrument() : ModulePass(ID) {
    PassRegistry &Registry = *PassRegistry::getPassRegistry();
    initializeLoopInfoWrapperPassPass(Registry);
}

void ArrayListInstrument::SetupTypes() {

    this->VoidType = Type::getVoidTy(pModule->getContext());
    this->LongType = IntegerType::get(pModule->getContext(), 64);
    this->IntType = IntegerType::get(pModule->getContext(), 32);
    this->VoidPointerType = PointerType::get(IntegerType::get(pModule->getContext(), 8), 0);

}

void ArrayListInstrument::SetupConstants() {

    // long
    this->ConstantLong0 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("0"), 10));
    this->ConstantLong1 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("1"), 10));
}

void ArrayListInstrument::SetupGlobals() {
//    assert(pModule->getGlobalVariable("numCost") == NULL);
    this->numCost = new GlobalVariable(*pModule, this->LongType, false, GlobalValue::ExternalLinkage, 0, "numCost");
    this->numCost->setAlignment(8);
    this->numCost->setInitializer(this->ConstantLong0);

}

void ArrayListInstrument::SetupFunctions() {

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
    this->aprof_dump = this->pModule->getFunction("aprof_dump");
//    assert(this->aprof_read != NULL);

    if (!this->aprof_dump) {
        ArgTypes.push_back(this->VoidPointerType);
        ArgTypes.push_back(this->IntType);
        FunctionType *AprofReadType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->aprof_dump = Function::Create(AprofReadType, GlobalValue::ExternalLinkage, "aprof_dump", this->pModule);
        this->aprof_dump->setCallingConv(CallingConv::C);
        ArgTypes.clear();
    }

    // aprof_return
    this->aprof_return = this->pModule->getFunction("aprof_return");
//    assert(this->aprof_return != NULL);

    if (!this->aprof_return) {
        ArgTypes.push_back(this->LongType);
        FunctionType *AprofReturnType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->aprof_return = Function::Create(AprofReturnType, GlobalValue::ExternalLinkage, "aprof_return",
                                              this->pModule);
        this->aprof_return->setCallingConv(CallingConv::C);
        ArgTypes.clear();
    }

}

void ArrayListInstrument::InstrumentInit(Instruction *firstInst) {

    CallInst *aprof_init_call = CallInst::Create(this->aprof_init, "", firstInst);
    aprof_init_call->setCallingConv(CallingConv::C);
    aprof_init_call->setTailCall(false);
    AttributeList int32_call_PAL;
    aprof_init_call->setAttributes(int32_call_PAL);

}

void ArrayListInstrument::InstrumentReturn(Instruction *returnInst) {
    std::vector<Value *> vecParams;
    LoadInst *bb_pLoad = new LoadInst(this->numCost, "", false, 8, returnInst);
    vecParams.push_back(bb_pLoad);
    CallInst *void_49 = CallInst::Create(this->aprof_return, vecParams, "", returnInst);
    void_49->setCallingConv(CallingConv::C);
    void_49->setTailCall(false);
    AttributeList void_PAL;
    void_49->setAttributes(void_PAL);
}

void ArrayListInstrument::InstrumentHooks(Function *pFunction, Instruction *loadInst) {

    LoadInst *pLoadInst = new LoadInst(this->numCost, "", false, 8, loadInst);
    BinaryOperator *pAdd = BinaryOperator::Create(Instruction::Add, pLoadInst,
                                                  this->ConstantLong1, "addNumCost",
                                                  loadInst);
    new StoreInst(pAdd, this->numCost, false, 8, loadInst);

    Value *var = loadInst->getOperand(0);

    DataLayout *dl = new DataLayout(this->pModule);
    Type *type_1 = var->getType()->getContainedType(0);

//    while (isa<PointerType>(type_1))
//        type_1 = type_1->getContainedType(0);

    if (type_1->isSized()) {
        ConstantInt *const_int6 = ConstantInt::get(
                this->pModule->getContext(),
                APInt(32, StringRef(std::to_string(dl->getTypeAllocSize(type_1))), 10));

        CastInst *ptr_50 = new BitCastInst(var, this->VoidPointerType,
                                           "", loadInst);
        std::vector<Value *> void_51_params;
        void_51_params.push_back(ptr_50);
        void_51_params.push_back(const_int6);
        CallInst *void_51 = CallInst::Create(this->aprof_dump, void_51_params, "", loadInst);
        void_51->setCallingConv(CallingConv::C);
        void_51->setTailCall(false);
        AttributeList void_PAL;
        void_51->setAttributes(void_PAL);

    } else {

        loadInst->dump();
        type_1->dump();
        assert(false);

    }

    for (Function::iterator BI = pFunction->begin(); BI != pFunction->end(); BI++) {

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

void ArrayListInstrument::SetupInit(Module &M) {
    // all set up operation
    this->pModule = &M;
    SetupTypes();
    SetupConstants();
    SetupGlobals();
    SetupFunctions();

}


bool ArrayListInstrument::runOnModule(Module &M) {

    SetupInit(M);

    Function *pFunction = searchFunctionByName(M, strFileName, strFuncName, uSrcLine);

    if (pFunction == NULL) {
        errs() << "Cannot find the input function\n";
        return false;
    }

    LoopInfo &LoopInfo = getAnalysis<LoopInfoWrapperPass>(*pFunction).getLoopInfo();

    Loop *pLoop = searchLoopByLineNo(pFunction, &LoopInfo, uSrcLine);

    set<Value *> setArrayValue;

    LoadInst *ploadInst = NULL;

    if (isArrayAccessLoop(pLoop, setArrayValue)) {
        errs() << "\nFOUND ARRAY 0 ACCESSING LOOP\n";

        set<Value *>::iterator itSetBegin = setArrayValue.begin();
        set<Value *>::iterator itSetEnd = setArrayValue.end();

        while (itSetBegin != itSetEnd) {
            (*itSetBegin)->dump();
            ploadInst = dyn_cast<LoadInst>(*itSetBegin);
            itSetBegin++;
            InstrumentHooks(pFunction, ploadInst);
        }

    } else if (isArrayAccessLoop1(pLoop, setArrayValue)) {
        errs() << "\nFOUND ARRAY 1 ACCESSING LOOP\n";

        set<Value *>::iterator itSetBegin = setArrayValue.begin();
        set<Value *>::iterator itSetEnd = setArrayValue.end();

        while (itSetBegin != itSetEnd) {
            (*itSetBegin)->dump();
            ploadInst = dyn_cast<LoadInst>(*itSetBegin);
            itSetBegin++;
            InstrumentHooks(pFunction, ploadInst);
        }
    }


    set<Value *> setLinkedValue;

    if (isLinkedListAccessLoop(pLoop, setLinkedValue)) {
        errs() << "\nFOUND Linked List ACCESSING LOOP\n";
        set<Value *>::iterator itSetBegin = setLinkedValue.begin();
        set<Value *>::iterator itSetEnd = setLinkedValue.end();

        while (itSetBegin != itSetEnd) {
            (*itSetBegin)->dump();
            ploadInst = dyn_cast<LoadInst>(*itSetBegin);
            itSetBegin++;
            InstrumentHooks(pFunction, ploadInst);
        }
    }

    if (ploadInst == NULL) {
        errs() << "Could not find Array or LinkedList in target Function!" << "\n";
        exit(0);
    }

    Function *MainFunc = this->pModule->getFunction("main");
    assert(MainFunc != NULL);

    if (MainFunc) {

        Instruction *firstInst = MainFunc->getEntryBlock().getFirstNonPHI();
        InstrumentInit(firstInst);
    }

    return false;
}

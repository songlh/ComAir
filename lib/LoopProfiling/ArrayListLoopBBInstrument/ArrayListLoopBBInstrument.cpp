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


#include "LoopProfiling/ArrayListLoopBBInstrument/ArrayListLoopBBInstrument.h"
#include "Common/ArrayLinkedIndentifier.h"
#include "Common/Helper.h"
#include "Common/Loop.h"


using namespace llvm;
using namespace std;


static RegisterPass<ArrayListLoopBBInstrument> X("array-list-loop-bb-instrument",
                                           "instrument a loop accessing an array element in each iteration",
                                           true, false);

char ArrayListLoopBBInstrument::ID = 0;



void ArrayListLoopBBInstrument::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesCFG();
    AU.addRequired<LoopInfoWrapperPass>();
}

ArrayListLoopBBInstrument::ArrayListLoopBBInstrument() : ModulePass(ID) {
    PassRegistry &Registry = *PassRegistry::getPassRegistry();
    initializeLoopInfoWrapperPassPass(Registry);
}

void ArrayListLoopBBInstrument::SetupTypes() {

    this->VoidType = Type::getVoidTy(pModule->getContext());
    this->LongType = IntegerType::get(pModule->getContext(), 64);
    this->IntType = IntegerType::get(pModule->getContext(), 32);
    this->VoidPointerType = PointerType::get(IntegerType::get(pModule->getContext(), 8), 0);

}

void ArrayListLoopBBInstrument::SetupConstants() {

    // long
    this->ConstantLong0 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("0"), 10));
    this->ConstantLong1 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("1"), 10));
}

void ArrayListLoopBBInstrument::SetupGlobals() {
//    assert(pModule->getGlobalVariable("numCost") == NULL);
//    this->numCost = new GlobalVariable(*pModule, this->LongType, false, GlobalValue::ExternalLinkage, 0, "numCost");
//    this->numCost->setAlignment(8);
//    this->numCost->setInitializer(this->ConstantLong1);

}

void ArrayListLoopBBInstrument::SetupFunctions() {

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
        ArgTypes.push_back(this->IntType);
        FunctionType *AprofReadType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->aprof_read = Function::Create(AprofReadType, GlobalValue::ExternalLinkage, "aprof_read", this->pModule);
        this->aprof_read->setCallingConv(CallingConv::C);
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

void ArrayListLoopBBInstrument::InstrumentInit(Instruction *firstInst) {

    CallInst *aprof_init_call = CallInst::Create(this->aprof_init, "", firstInst);
    aprof_init_call->setCallingConv(CallingConv::C);
    aprof_init_call->setTailCall(false);
    AttributeList int32_call_PAL;
    aprof_init_call->setAttributes(int32_call_PAL);

}

void ArrayListLoopBBInstrument::InstrumentReturn(Function *F) {

    for (Function::iterator BI = F->begin(); BI != F->end(); BI++) {

        BasicBlock *BB = &*BI;

        for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {

            Instruction *Inst = &*II;

            switch (Inst->getOpcode()) {

                case Instruction::Ret: {
                    std::vector<Value *> vecParams;
                    LoadInst *bb_pLoad = new LoadInst(this->BBAllocInst, "", false, 8, Inst);
                    vecParams.push_back(bb_pLoad);
                    CallInst *void_49 = CallInst::Create(this->aprof_return, vecParams, "", Inst);
                    void_49->setCallingConv(CallingConv::C);
                    void_49->setTailCall(false);
                    AttributeList void_PAL;
                    void_49->setAttributes(void_PAL);
                    break;
                }
            }
        }
    }

}

void ArrayListLoopBBInstrument::InstrumentFinal(Function *mainFunc) {

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

void ArrayListLoopBBInstrument::InstrumentCostUpdater(Loop *pLoop) {
    Function *pFunction = pLoop->getHeader()->getParent();
    Instruction *pInstBefore = &*pFunction->getEntryBlock().getFirstInsertionPt();

    this->BBAllocInst = new AllocaInst(this->LongType, 0, "NumCost", pInstBefore);
    this->BBAllocInst->setAlignment(8);
    StoreInst *pStore = new StoreInst(this->ConstantLong0, this->BBAllocInst, false, pInstBefore);
    pStore->setAlignment(8);

   for(Loop::block_iterator BB = pLoop->block_begin(); BB != pLoop->block_end(); BB++) {
        BasicBlock *pBlock = *BB;
        Instruction *pInst = &*pBlock->getFirstInsertionPt();
        LoadInst *pLoadnumCost = new LoadInst(this->BBAllocInst, "", false, pInst);
        pLoadnumCost->setAlignment(8);
        BinaryOperator *pAdd = BinaryOperator::Create(Instruction::Add, pLoadnumCost,
                                                      this->ConstantLong1, "add",
                                                      pInst);
        StoreInst *pStore = new StoreInst(pAdd, this->BBAllocInst, false, pInst);
        pStore->setAlignment(8);

    }

}


void ArrayListLoopBBInstrument::InstrumentCallBefore(Function *F) {
    int FuncID = GetFunctionID(F);
    std::vector<Value *> vecParams;
    Instruction *firstInst = &*(F->getEntryBlock().getFirstNonPHI());

    ConstantInt *const_int6 = ConstantInt::get(
            this->pModule->getContext(),
            APInt(32, StringRef(std::to_string(FuncID)), 10));

    vecParams.push_back(const_int6);

    CallInst *void_46 = CallInst::Create(
            this->aprof_call_before, vecParams, "", firstInst);
    void_46->setCallingConv(CallingConv::C);
    void_46->setTailCall(false);
    AttributeList void_PAL;
    void_46->setAttributes(void_PAL);

}


void ArrayListLoopBBInstrument::InstrumentHooks(Function *pFunction, Instruction *pInst) {

    Value *var = pInst->getOperand(0);

    DataLayout *dl = new DataLayout(this->pModule);
    Type *type_1 = var->getType()->getContainedType(0);

    if (type_1->isSized()) {
        ConstantInt *const_int6 = ConstantInt::get(
                this->pModule->getContext(),
                APInt(32, StringRef(std::to_string(dl->getTypeAllocSize(type_1))), 10));

        CastInst *ptr_50 = new BitCastInst(var, this->VoidPointerType,
                                           "", pInst);
        std::vector<Value *> void_51_params;
        void_51_params.push_back(ptr_50);
        void_51_params.push_back(const_int6);
        CallInst *void_51 = CallInst::Create(this->aprof_read, void_51_params, "", pInst);
        void_51->setCallingConv(CallingConv::C);
        void_51->setTailCall(false);
        AttributeList void_PAL;
        void_51->setAttributes(void_PAL);

    } else {

        pInst->dump();
        type_1->dump();
        assert(false);

    }

}

void ArrayListLoopBBInstrument::SetupInit(Module &M) {
    // all set up operation
    this->pModule = &M;
    SetupTypes();
    SetupConstants();
    SetupGlobals();
    SetupFunctions();

}


bool ArrayListLoopBBInstrument::runOnModule(Module &M) {

    SetupInit(M);
    std::set<Loop *> LoopSet;

    //identify loops, and store loop-header into HeaderSet
    for (Module::iterator FI = M.begin(); FI != M.end(); FI++) {

        Function *F = &*FI;
        if (F->empty())
            continue;

        //clear loop set
        LoopSet.clear();

        LoopInfo &LoopInfo = getAnalysis<LoopInfoWrapperPass>(*F).getLoopInfo();

        if (LoopInfo.empty()) {
            continue;
        }

        for (auto &loop:LoopInfo) {
            //LoopSet.insert(loop);
            std::set<Loop *> tempSet = getSubLoopSet(loop); //including the loop itself
            LoopSet.insert(tempSet.begin(), tempSet.end());
        }

        if (LoopSet.empty()) {
            continue;
        }

        InstrumentCallBefore(F);

        for (Loop *loop:LoopSet) {

            if (loop == nullptr)
                continue;

            // Array
            set<Value *> setArrayValue;
            bool isAAL_0 = isArrayAccessLoop(loop, setArrayValue);
            Instruction *pInst = NULL;

            if (isAAL_0) {
                errs() << "FOUND ARRAY 0 ACCESSING LOOP\n";
                set<Value *>::iterator itSetBegin = setArrayValue.begin();
                set<Value *>::iterator itSetEnd = setArrayValue.end();

                while (itSetBegin != itSetEnd) {
//                    (*itSetBegin)->dump();
                    pInst = dyn_cast<Instruction>(*itSetBegin);
                    itSetBegin++;
                    InstrumentHooks(F, pInst);
                }
            }

            bool isAAL_1 = isArrayAccessLoop1(loop, setArrayValue);
            if (isAAL_1) {
                errs() << "FOUND ARRAY 1 ACCESSING LOOP\n";
                set<Value *>::iterator itSetBegin = setArrayValue.begin();
                set<Value *>::iterator itSetEnd = setArrayValue.end();

                while (itSetBegin != itSetEnd) {
//                    (*itSetBegin)->dump();
                    pInst = dyn_cast<Instruction>(*itSetBegin);
                    itSetBegin++;
                    InstrumentHooks(F, pInst);
                }

            }

            // linkedList
            set<Value *> setLinkedValue;

            bool isLLAL = isLinkedListAccessLoop(loop, setLinkedValue);
            if (isLLAL) {
                errs() << "FOUND Linked List ACCESSING LOOP\n";
                set<Value *>::iterator itSetBegin = setLinkedValue.begin();
                set<Value *>::iterator itSetEnd = setLinkedValue.end();

                while (itSetBegin != itSetEnd) {
//                    (*itSetBegin)->dump();
                    pInst = dyn_cast<Instruction>(*itSetBegin);
                    itSetBegin++;
                    InstrumentHooks(F, pInst);
                }
            }

            // instrument loop basic blocks
            InstrumentCostUpdater(loop);
        }

        InstrumentReturn(F);
    }


    Function *MainFunc = this->pModule->getFunction("main");
    assert(MainFunc != NULL);

    if (MainFunc) {

        Instruction *firstInst = MainFunc->getEntryBlock().getFirstNonPHI();
        InstrumentInit(firstInst);
        InstrumentFinal(MainFunc);
    }

    return false;
}

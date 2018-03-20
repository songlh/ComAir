#include <set>

#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Support/raw_ostream.h"

#include "Sampling/LoopInterestedHook/LoopInterestedHook.h"
#include "Common/Constant.h"
#include "Common/Helper.h"
#include "Common/Loop.h"
#include "Common/Search.h"

#include <map>

using namespace llvm;
using namespace std;


static RegisterPass<LoopInterestedHook> X(
        "interested-loop-hook",
        "only instrument hook in interested loop",
        false, false);


static cl::opt<unsigned> uInnerSrcLine("noInnerLine",
                                       cl::desc("Source Code Line Number for Inner Loop"), cl::Optional,
                                       cl::value_desc("uInnerSrcCodeLine"));


static cl::opt<std::string> strInnerFuncName("strInnerFunc",
                                             cl::desc("Function Name"), cl::Optional,
                                             cl::value_desc("strInnerFuncName"));

static cl::opt<unsigned> uOuterSrcLine("noOuterLine",
                                       cl::desc("Source Code Line Number for Outer Loop"), cl::Optional,
                                       cl::value_desc("uSrcOuterCodeLine"));

static cl::opt<std::string> strOuterFuncName("strOuterFunc",
                                             cl::desc("Function Name for Outer Loop"), cl::Optional,
                                             cl::value_desc("strOuterFuncName"));

static cl::opt<std::string> strMonitorInstFile("strInstFile",
                                               cl::desc("Monitored Insts File Name"), cl::Optional,
                                               cl::value_desc("strInstFile"));

static cl::opt<std::string> strMainFunc("strMain",
                                        cl::desc("Main Function"), cl::Optional,
                                        cl::value_desc("strMain"));

static cl::opt<std::string> strLoopHeader("strLoopHeader",
                                          cl::desc("Block Name for Inner Loop"), cl::Optional,
                                          cl::value_desc("strLoopHeader"));

static cl::opt<std::string> strLibrary("strLibrary",
                                       cl::desc("File Name for libraries"), cl::Optional,
                                       cl::value_desc("strLibrary"));

static cl::opt<int> SamplingRate("sampleRate",
                                 cl::desc("The rate of sampling."),
                                 cl::init(100));


/* local function */

bool isI8PointerType(Value *val) {

    Type *targetType = val->getType();
    if (!targetType->isPointerTy()) {
        return false;
    }

    if (IntegerType *IT = dyn_cast<IntegerType>(targetType->getPointerElementType())) {
        if (IT->getBitWidth() == 8)
            return true;
    }

    return false;

}


/* */

char LoopInterestedHook::ID = 0;

void LoopInterestedHook::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<PostDominatorTreeWrapperPass>();
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<ScalarEvolutionWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
}

LoopInterestedHook::LoopInterestedHook() : ModulePass(ID) {
    PassRegistry &Registry = *PassRegistry::getPassRegistry();
    initializeScalarEvolutionWrapperPassPass(Registry);
    initializeLoopInfoWrapperPassPass(Registry);
    initializePostDominatorTreeWrapperPassPass(Registry);
    initializeDominatorTreeWrapperPassPass(Registry);
}

void LoopInterestedHook::SetupTypes() {

    this->VoidType = Type::getVoidTy(pModule->getContext());
    this->IntType = IntegerType::get(pModule->getContext(), 32);
    this->LongType = IntegerType::get(pModule->getContext(), 64);
    this->VoidPointerType = PointerType::get(IntegerType::get(pModule->getContext(), 8), 0);

}

void LoopInterestedHook::SetupConstants() {

    // long
    this->ConstantLong0 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("0"), 10));
    this->ConstantLong1 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("1"), 10));

    // int
    this->ConstantInt0 = ConstantInt::get(pModule->getContext(), APInt(32, StringRef("0"), 10));
    this->ConstantInt1 = ConstantInt::get(pModule->getContext(), APInt(32, StringRef("1"), 10));
    this->ConstantInt2 = ConstantInt::get(pModule->getContext(), APInt(32, StringRef("2"), 10));
    this->ConstantInt3 = ConstantInt::get(pModule->getContext(), APInt(32, StringRef("3"), 10));
    this->ConstantSamplingRate = ConstantInt::get(pModule->getContext(),
                                                  APInt(32, StringRef(std::to_string(SamplingRate)), 10));

}

void LoopInterestedHook::SetupGlobals() {
    assert(pModule->getGlobalVariable("numCost") == NULL);
    this->numCost = new GlobalVariable(*pModule, this->LongType, false, GlobalValue::ExternalLinkage, 0, "numCost");
    this->numCost->setAlignment(8);
    this->numCost->setInitializer(this->ConstantLong0);

    assert(pModule->getGlobalVariable("Switcher") == NULL);
    this->Switcher = new GlobalVariable(*pModule, this->IntType,
                                        false, GlobalValue::ExternalLinkage,
                                        0, "Switcher");
    this->Switcher->setAlignment(4);
    this->Switcher->setInitializer(this->ConstantInt0);

    assert(pModule->getGlobalVariable("GeoRate") == NULL);
    this->GeoRate = new GlobalVariable(*pModule, this->IntType,
                                       false, GlobalValue::ExternalLinkage,
                                       0, "GeoRate");
    this->GeoRate->setAlignment(4);
    this->GeoRate->setInitializer(this->ConstantSamplingRate);

    assert(pModule->getGlobalVariable("SampleMonitor") == NULL);
    this->SampleMonitor = new GlobalVariable(*pModule, this->IntType,
                                             false, GlobalValue::ExternalLinkage,
                                             0, "SampleMonitor");
    this->SampleMonitor->setAlignment(4);
    this->SampleMonitor->setInitializer(this->ConstantInt2);
}

void LoopInterestedHook::SetupFunctions() {

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

void LoopInterestedHook::RemapInstruction(Instruction *I, ValueToValueMapTy &VMap) {
    for (unsigned op = 0, E = I->getNumOperands(); op != E; ++op) {
        Value *Op = I->getOperand(op);
        ValueToValueMapTy::iterator It = VMap.find(Op);
        if (It != VMap.end()) {
            I->setOperand(op, It->second);
        }
    }

    if (PHINode *PN = dyn_cast<PHINode>(I)) {
        for (unsigned i = 0, e = PN->getNumIncomingValues(); i != e; ++i) {
            ValueToValueMapTy::iterator It = VMap.find(PN->getIncomingBlock(i));
            if (It != VMap.end())
                PN->setIncomingBlock(i, cast<BasicBlock>(It->second));
        }
    }
}

void LoopInterestedHook::InstrumentCostUpdater(Loop *pLoop) {
    BasicBlock *pHeader = pLoop->getHeader();
    LoadInst *pLoadnumGlobalCounter = NULL;
    BinaryOperator *pAdd = NULL;
    StoreInst *pStore = NULL;

    pLoadnumGlobalCounter = new LoadInst(this->numCost, "", false, pHeader->getTerminator());
    pLoadnumGlobalCounter->setAlignment(8);
    pAdd = BinaryOperator::Create(Instruction::Add, pLoadnumGlobalCounter, this->ConstantLong1, "add",
                                  pHeader->getTerminator());
    pStore = new StoreInst(pAdd, this->numCost, false, pHeader->getTerminator());
    pStore->setAlignment(8);
}

void LoopInterestedHook::AddHooksToOuterFunction(Function *Func) {

    if (Func) {

        for (Function::iterator BI = Func->begin(); BI != Func->end(); ++BI) {

            BasicBlock *BB = &*BI;

            for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {

                Instruction *Inst = &*II;

                switch (Inst->getOpcode()) {

                    case Instruction::Ret: {
                        std::vector<Value *> vecParams;
                        LoadInst *bb_pLoad = new LoadInst(this->numCost, "", false, 8, Inst);
                        vecParams.push_back(bb_pLoad);
                        vecParams.push_back(this->ConstantInt3);
                        CallInst *void_49 = CallInst::Create(this->aprof_return, vecParams, "", Inst);
                        void_49->setCallingConv(CallingConv::C);
                        void_49->setTailCall(false);
                        AttributeList void_PAL;
                        void_49->setAttributes(void_PAL);
                    }
                        break;
                }
            }
        }

    } else {

        errs() << "Could not find outer loop function!" << "\n";
    }

}

void LoopInterestedHook::InstrumentOuterLoop(Loop *pOuterLoop) {
    InstrumentCostUpdater(pOuterLoop);
//    AddHooksToOuterFunction(pOuterLoop->getHeader()->getParent());

}

void LoopInterestedHook::InstrumentInit(Instruction *firstInst) {

    CallInst *aprof_init_call = CallInst::Create(this->aprof_init, "", firstInst);
    aprof_init_call->setCallingConv(CallingConv::C);
    aprof_init_call->setTailCall(false);
    AttributeList int32_call_PAL;
    aprof_init_call->setAttributes(int32_call_PAL);

}

void LoopInterestedHook::InstrumentRead(LoadInst *pLoad, Instruction *BeforeInst) {

    Value *var = pLoad->getOperand(0);

    if (var->getName().str() == "numCost") {
        return;
    }

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
//        LoadInst *pLoad = new LoadInst(this->SampleMonitor, "", false, 4, BeforeInst);
        std::vector<Value *> void_51_params;
        void_51_params.push_back(ptr_50);
        void_51_params.push_back(const_int6);
//        void_51_params.push_back(pLoad);
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

void LoopInterestedHook::InstrumentWrite(StoreInst *pStore, Instruction *BeforeInst) {

    Value *var = pStore->getOperand(1);

    if (var->getName().str() == "numCost") {
        return;
    }

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
//        LoadInst *pLoad = new LoadInst(this->SampleMonitor, "", false, 4, BeforeInst);

        std::vector<Value *> void_51_params;
        void_51_params.push_back(ptr_50);
        void_51_params.push_back(const_int6);
//        void_51_params.push_back(pLoad);

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

void LoopInterestedHook::InstrumentReturn(Instruction *BeforeInst) {

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

    // switcher--;
    pLoad = new LoadInst(this->Switcher, "", false, 4, BeforeInst);
    BinaryOperator *int32_dec = BinaryOperator::Create(
            Instruction::Sub, pLoad,
            this->ConstantInt1, "dec", BeforeInst);

    new StoreInst(int32_dec, this->Switcher, false, 4, BeforeInst);
}

void LoopInterestedHook::CloneInnerLoop(Loop *pLoop, vector<BasicBlock *> &vecAdd, ValueToValueMapTy &VMap) {
    Function *pFunction = pLoop->getHeader()->getParent();

    SmallVector<BasicBlock *, 4> ExitBlocks;
    pLoop->getExitBlocks(ExitBlocks);

    set<BasicBlock *> setExitBlocks;

    for (unsigned long i = 0; i < ExitBlocks.size(); i++) {
        setExitBlocks.insert(ExitBlocks[i]);
    }

    for (unsigned long i = 0; i < ExitBlocks.size(); i++) {
        VMap[ExitBlocks[i]] = ExitBlocks[i];
    }

    vector<BasicBlock *> ToClone;
    vector<BasicBlock *> BeenCloned;

    set<BasicBlock *> setCloned;

    //clone loop
    ToClone.push_back(pLoop->getHeader());

    while (ToClone.size() > 0) {

        BasicBlock *pCurrent = ToClone.back();
        ToClone.pop_back();

        WeakTrackingVH &BBEntry = VMap[pCurrent];
        if (BBEntry) {
            continue;
        }

        BasicBlock *NewBB;
        BBEntry = NewBB = BasicBlock::Create(pCurrent->getContext(), "", pFunction);

        if (pCurrent->hasName()) {
            NewBB->setName(pCurrent->getName() + ".CPI");
        }

        if (pCurrent->hasAddressTaken()) {
            errs() << "hasAddressTaken branch\n";
            exit(0);
        }

        for (BasicBlock::iterator II = pCurrent->begin(); II != pCurrent->end(); ++II) {
            Instruction *Inst = &*II;
            Instruction *NewInst = Inst->clone();
            if (II->hasName()) {
                NewInst->setName(II->getName() + ".CPI");
            }
            VMap[Inst] = NewInst;
            NewBB->getInstList().push_back(NewInst);
        }

        const TerminatorInst *TI = pCurrent->getTerminator();
        for (unsigned i = 0, e = TI->getNumSuccessors(); i != e; ++i) {
            ToClone.push_back(TI->getSuccessor(i));
        }

        setCloned.insert(pCurrent);
        BeenCloned.push_back(NewBB);
    }

    //remap value used inside loop
    vector<BasicBlock *>::iterator itVecBegin = BeenCloned.begin();
    vector<BasicBlock *>::iterator itVecEnd = BeenCloned.end();


    for (; itVecBegin != itVecEnd; itVecBegin++) {
        for (BasicBlock::iterator II = (*itVecBegin)->begin(); II != (*itVecBegin)->end(); II++) {
            this->RemapInstruction(&*II, VMap);
        }
    }

    //add to the else if body
    BasicBlock *pCondition1 = vecAdd[0];
    BasicBlock *pElseBody = vecAdd[2];
//    BasicBlock *pElseBody = vecAdd[6];

    BasicBlock *pClonedHeader = cast<BasicBlock>(VMap[pLoop->getHeader()]);

    BranchInst::Create(pClonedHeader, pElseBody);

    for (BasicBlock::iterator II = pClonedHeader->begin(); II != pClonedHeader->end(); II++) {
        if (PHINode *pPHI = dyn_cast<PHINode>(II)) {
            vector<int> vecToRemoved;
            for (unsigned i = 0, e = pPHI->getNumIncomingValues(); i != e; ++i) {
                if (pPHI->getIncomingBlock(i) == pCondition1) {
                    pPHI->setIncomingBlock(i, pElseBody);
                }

            }

        }
    }

    set<BasicBlock *> setProcessedBlock;

    for (unsigned long i = 0; i < ExitBlocks.size(); i++) {

        if (setProcessedBlock.find(ExitBlocks[i]) != setProcessedBlock.end()) {
            continue;

        } else {

            setProcessedBlock.insert(ExitBlocks[i]);
        }

        for (BasicBlock::iterator II = ExitBlocks[i]->begin(); II != ExitBlocks[i]->end(); II++) {

            if (PHINode *pPHI = dyn_cast<PHINode>(II)) {

                unsigned numIncomming = pPHI->getNumIncomingValues();

                for (unsigned i = 0; i < numIncomming; i++) {

                    BasicBlock *incommingBlock = pPHI->getIncomingBlock(i);

                    if (VMap.find(incommingBlock) != VMap.end()) {

                        Value *incommingValue = pPHI->getIncomingValue(i);

                        if (VMap.find(incommingValue) != VMap.end()) {
                            incommingValue = VMap[incommingValue];
                        }

                        pPHI->addIncoming(incommingValue, cast<BasicBlock>(VMap[incommingBlock]));

                    }
                }

            }
        }
    }
}

//void LoopInterestedHook::CreateIfElseIfBlock(Loop *pInnerLoop, vector<BasicBlock *> &vecAdded) {
//    BasicBlock *pCondition1 = NULL;
//    BasicBlock *pCondition2 = NULL;
//    BasicBlock *pElseBody = NULL;
//    LoadInst *pLoad1 = NULL;
//    LoadInst *pLoad2 = NULL;
//    ICmpInst *pCmp = NULL;
//
//    BinaryOperator *pBinary = NULL;
//    TerminatorInst *pTerminator = NULL;
//    BranchInst *pBranch = NULL;
//    StoreInst *pStore = NULL;
//    CallInst *pCall = NULL;
//    AttributeList emptyList;
//
//    Function *pInnerFunction = pInnerLoop->getHeader()->getParent();
//    Module *pModule = pInnerFunction->getParent();
//
//    pCondition1 = pInnerLoop->getLoopPreheader();
//    BasicBlock *pHeader = pInnerLoop->getHeader();
//
//    pCondition2 = BasicBlock::Create(pModule->getContext(), ".if2.aprof", pInnerFunction, 0);
//    BasicBlock *label_if_then = BasicBlock::Create(pModule->getContext(), "if.then", pInnerFunction, 0);
//    BasicBlock *label_if_then3 = BasicBlock::Create(pModule->getContext(), "if.then3", pInnerFunction, 0);
//    BasicBlock *label_if_end = BasicBlock::Create(pModule->getContext(), "if.end", pInnerFunction, 0);
//    BasicBlock *label_if_then5 = BasicBlock::Create(pModule->getContext(), "if.then5", pInnerFunction, 0);
//    pElseBody = BasicBlock::Create(pModule->getContext(), ".else.body.aprof", pInnerFunction, 0);
//
//    pTerminator = pCondition1->getTerminator();
//
//    if (!this->bGivenOuterLoop) {
//        assert(false);
//    }
//
//    pLoad1 = new LoadInst(this->SampleMonitor, "", false, pTerminator);
//    pLoad1->setAlignment(4);
//    pCmp = new ICmpInst(pTerminator, ICmpInst::ICMP_SLT, pLoad1, this->ConstantInt2, "cmp0");
//    pBranch = BranchInst::Create(label_if_then, pCondition2, pCmp);
//    ReplaceInstWithInst(pTerminator, pBranch);
//
//    //condition2
//    pLoad2 = new LoadInst(this->Switcher, "", false, 4, pCondition2);
//    pCmp = new ICmpInst(*pCondition2, ICmpInst::ICMP_EQ, pLoad2, this->ConstantInt0, "cmp1");
//    BranchInst::Create(label_if_then, pHeader, pCmp, pCondition2);
//
//    // Block if.then (label_if_then)
//    pLoad1 = new LoadInst(this->SampleMonitor, "", false, 4, label_if_then);
//    pBinary = BinaryOperator::Create(Instruction::Sub, pLoad1,
//                                     this->ConstantInt1, "dec", label_if_then);
//    new StoreInst(pBinary, this->SampleMonitor, false, 4, label_if_then);
//
//    pLoad1 = new LoadInst(this->SampleMonitor, "", false, 4, label_if_then);
//    pCmp = new ICmpInst(*label_if_then, ICmpInst::ICMP_EQ, pLoad1, this->ConstantInt0, "cmp2");
//    BranchInst::Create(label_if_then3, label_if_end, pCmp, label_if_then);
//
//    // Block if.then3 (label_if_then3)
//    pStore = new StoreInst(this->ConstantInt2, this->SampleMonitor, false, 4, label_if_then3);
//    BranchInst::Create(label_if_end, label_if_then3);
//
//    // Block if.end (label_if_end)
//    LoadInst *int32_35 = new LoadInst(this->Switcher, "", false, 4, label_if_end);
//    ICmpInst *int1_cmp4 = new ICmpInst(*label_if_end, ICmpInst::ICMP_EQ, int32_35, this->ConstantInt0, "cmp3");
//    BranchInst::Create(label_if_then5, pElseBody, int1_cmp4, label_if_end);
//
//    // Block if.then5 (label_if_then5)
//    pLoad1 = new LoadInst(this->GeoRate, "", false, label_if_then5);
//    pLoad1->setAlignment(4);
//    pCall = CallInst::Create(this->aprof_geo, pLoad1, "", label_if_then5);
//    pCall->setCallingConv(CallingConv::C);
//    pCall->setTailCall(false);
//    pCall->setAttributes(emptyList);
//    new StoreInst(pCall, this->Switcher, false, 4, label_if_then5);
//    BranchInst::Create(pElseBody, label_if_then5);
//
//
//    vecAdded.push_back(pCondition1);
//    vecAdded.push_back(pCondition2);
//    vecAdded.push_back(label_if_then);
//    vecAdded.push_back(label_if_then3);
//    vecAdded.push_back(label_if_end);
//    vecAdded.push_back(label_if_then5);
//    // num 6
//    vecAdded.push_back(pElseBody);
//
//}

void LoopInterestedHook::CreateIfElseIfBlock(Loop *pInnerLoop, vector<BasicBlock *> &vecAdded) {
    BasicBlock *pCondition1 = NULL;
    BasicBlock *pElseBody = NULL;
    BranchInst *pBranch = NULL;
    LoadInst *pLoad1 = NULL;
    LoadInst *pLoad2 = NULL;
    ICmpInst *pCmp = NULL;

    TerminatorInst *pTerminator = NULL;
    CallInst *pCall = NULL;
    AttributeList emptyList;

    Function *pInnerFunction = pInnerLoop->getHeader()->getParent();
    Module *pModule = pInnerFunction->getParent();

    pCondition1 = pInnerLoop->getLoopPreheader();
    BasicBlock *pHeader = pInnerLoop->getHeader();

    BasicBlock *label_geo = BasicBlock::Create(pModule->getContext(), "label.geo", pInnerFunction, 0);
    pElseBody = BasicBlock::Create(pModule->getContext(), ".else.body.aprof", pInnerFunction, 0);

    pTerminator = pCondition1->getTerminator();

    if (!this->bGivenOuterLoop) {
        assert(false);
    }

    //condition2
    pLoad2 = new LoadInst(this->Switcher, "", false, 4, pTerminator);
    pCmp = new ICmpInst(pTerminator, ICmpInst::ICMP_EQ, pLoad2, this->ConstantInt0, "cmp1");
    pBranch = BranchInst::Create(label_geo, pHeader, pCmp);
    ReplaceInstWithInst(pTerminator, pBranch);

    // Block if.then5 (label_if_then5)
    pLoad1 = new LoadInst(this->GeoRate, "", false, 4, label_geo);
    pCall = CallInst::Create(this->aprof_geo, pLoad1, "", label_geo);
    pCall->setCallingConv(CallingConv::C);
    pCall->setTailCall(false);
    pCall->setAttributes(emptyList);
    new StoreInst(pCall, this->Switcher, false, 4, label_geo);
    BranchInst::Create(pElseBody, label_geo);

    vecAdded.push_back(pCondition1);
    vecAdded.push_back(label_geo);
    // num 2
    vecAdded.push_back(pElseBody);

}

void LoopInterestedHook::AddHooksToInnerFunction(Function *pInnerFunction) {

    if (!pInnerFunction) {
        assert(false);
    }

    for (Function::iterator BI = pInnerFunction->begin(); BI != pInnerFunction->end(); ++BI) {

        BasicBlock *BB = &*BI;
        std::string BBName = BB->getName().str();


        for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {

            Instruction *Inst = &*II;

            switch (Inst->getOpcode()) {

                case Instruction::Load: {
                    if (BBName.find(".CPI") != std::string::npos) {
                        if (LoadInst *pLoad = dyn_cast<LoadInst>(Inst)) {
                            InstrumentRead(pLoad, Inst);
                        }
                    }
                    break;
                }

//                case Instruction::Store: {
//                    if (BBName.find(".CPI") != std::string::npos) {
//                        if (StoreInst *pStore = dyn_cast<StoreInst>(Inst)) {
//                            InstrumentWrite(pStore, Inst);
//                        }
//                    }
//
//                    break;
//                }

                case Instruction::Ret: {
                    InstrumentReturn(Inst);
                }
                    break;
            }
        }
    }
}

void LoopInterestedHook::InstrumentInnerLoop(Loop *pInnerLoop, PostDominatorTree *PDT) {
    set<BasicBlock *> setBlocksInLoop;

    for (Loop::block_iterator BB = pInnerLoop->block_begin(); BB != pInnerLoop->block_end(); BB++) {
        setBlocksInLoop.insert(*BB);
    }

    InstrumentCostUpdater(pInnerLoop);

    vector<LoadInst *> vecLoad;
    vector<Instruction *> vecIn;
    vector<Instruction *> vecOut;
    vector<MemTransferInst *> vecMem;

    //created auxiliary basic block
    vector<BasicBlock *> vecAdd;
    CreateIfElseIfBlock(pInnerLoop, vecAdd);

    //clone loop
    ValueToValueMapTy VMap;
    CloneInnerLoop(pInnerLoop, vecAdd, VMap);

    //add loop related hooks
    AddHooksToInnerFunction(pInnerLoop->getHeader()->getParent());

}

void LoopInterestedHook::SetupInit() {
    // all set up operation
    SetupTypes();
    SetupConstants();
    SetupGlobals();
    SetupFunctions();

}

void LoopInterestedHook::SetupHooks() {

    for (Module::iterator FI = this->pModule->begin(); FI != this->pModule->end(); FI++) {

        Function *Func = &*FI;

        if (IsIgnoreFunc(Func)) {
            continue;

        } else if (this->funNameIDFile)
            this->funNameIDFile << Func->getName().str()
                                << ":" << GetFunctionID(Func) << "\n";


        InstrumentHooks(Func, !getIgnoreOptimizedFlag(Func));

    }

    Function *MainFunc = this->pModule->getFunction("main");
    assert(MainFunc != NULL);

    if (MainFunc) {

        //Instruction *firstInst = &*(MainFunc->begin()->begin());
        Instruction *firstInst = MainFunc->getEntryBlock().getFirstNonPHI();
        InstrumentInit(firstInst);

        // this is make sure main function has call before,
        // so the shadow stack will not empty in running.
    }
}

bool LoopInterestedHook::runOnModule(Module &M) {

    this->pModule = &M;
    SetupInit();

    if (strOuterFuncName != "") {

        Function *pOuterFunction = SearchFunctionByName(M, strOuterFuncName);

        if (pOuterFunction == NULL) {
            errs() << "Cannot find the function containing the outer loop!\n";
            return false;
        }

//        pOuterFunction->dump();

        LoopInfo *pOuterLI = &getAnalysis<LoopInfoWrapperPass>(*pOuterFunction).getLoopInfo();
        this->pOuterLoop = SearchLoopByLineNo(pOuterFunction, pOuterLI, uOuterSrcLine);

        if (pOuterLoop == NULL) {
            errs() << "Cannot find the outer loop!\n";
            return false;
        }

        this->bGivenOuterLoop = true;

    } else {

        this->bGivenOuterLoop = false;
    }


    if (this->bGivenOuterLoop) {

        InstrumentOuterLoop(this->pOuterLoop);
    }

    Function *pInnerFunction = SearchFunctionByName(M, strInnerFuncName);

    if (pInnerFunction == NULL) {
        errs() << "Cannot find the function containing the inner loop!\n";
        return true;
    }

//    ParseInstFile(pInnerFunction, &M);

    PostDominatorTree *PDT = &(getAnalysis<PostDominatorTreeWrapperPass>(*pInnerFunction).getPostDomTree());
    DominatorTree *DT = &(getAnalysis<DominatorTreeWrapperPass>(*pInnerFunction).getDomTree());
    LoopInfo &pInnerLI = (getAnalysis<LoopInfoWrapperPass>(*pInnerFunction).getLoopInfo());
    Loop *pInnerLoop;

    if (strLoopHeader == "") {

        pInnerLoop = SearchLoopByLineNo(pInnerFunction, &pInnerLI, uInnerSrcLine);

    } else {

        BasicBlock *pHeader = SearchBlockByName(pInnerFunction, strLoopHeader);

        if (pHeader == NULL) {
            errs() << "Cannot find the given loop header!\n";
            return true;
        }

        pInnerLoop = pInnerLI.getLoopFor(pHeader);
    }

    if (pInnerLoop == NULL) {

        errs() << "Cannot find the inner loop!\n";
        return true;
    }

    BasicBlock *pInnerHeader = pInnerLoop->getHeader();

    LoopSimplify(pInnerLoop, DT);
    // use loop-simplify pass

    InstrumentInnerLoop(pInnerLoop, PDT);
//    pInnerFunction->dump();

    Function *MainFunc = this->pModule->getFunction("main");
    assert(MainFunc != NULL);

    if (MainFunc) {

        Instruction *firstInst = MainFunc->getEntryBlock().getFirstNonPHI();
        InstrumentInit(firstInst);
        AddHooksToOuterFunction(MainFunc);
    }

    return true;
}

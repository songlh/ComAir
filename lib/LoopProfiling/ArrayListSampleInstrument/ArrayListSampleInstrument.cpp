#include <map>
#include <set>
#include <vector>

#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"


#include "LoopProfiling/ArrayListSampleInstrument/ArrayListSampleInstrument.h"
#include "Common/ArrayLinkedIndentifier.h"
#include "Common/Constant.h"
#include "Common/Loop.h"

using namespace llvm;
using namespace std;


static RegisterPass<ArrayListSampleInstrument> X("array-list-sample-instrument",
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


static cl::opt<int> SamplingRate("sampleRate",
                                 cl::desc("The rate of sampling."),
                                 cl::init(100));


char ArrayListSampleInstrument::ID = 0;


bool hasMarkFlag(Instruction *Inst) {
    MDNode *Node = Inst->getMetadata(ARRAY_LIST_INSERT);
    if (!Node) {
        return false;
    }

    assert(Node->getNumOperands() == 1);
    const Metadata *MD = Node->getOperand(0);
    if (auto *MDV = dyn_cast<ValueAsMetadata>(MD)) {
        Value *V = MDV->getValue();
        ConstantInt *CI = dyn_cast<ConstantInt>(V);
        assert(CI);
        return true;
    }

    return false;
}


void ArrayListSampleInstrument::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<PostDominatorTreeWrapperPass>();
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<ScalarEvolutionWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
}

ArrayListSampleInstrument::ArrayListSampleInstrument() : ModulePass(ID) {
    PassRegistry &Registry = *PassRegistry::getPassRegistry();
    initializeScalarEvolutionWrapperPassPass(Registry);
    initializeLoopInfoWrapperPassPass(Registry);
    initializePostDominatorTreeWrapperPassPass(Registry);
    initializeDominatorTreeWrapperPassPass(Registry);
}

void ArrayListSampleInstrument::SetupTypes() {

    this->VoidType = Type::getVoidTy(pModule->getContext());
    this->LongType = IntegerType::get(pModule->getContext(), 64);
    this->IntType = IntegerType::get(pModule->getContext(), 32);
    this->VoidPointerType = PointerType::get(IntegerType::get(pModule->getContext(), 8), 0);

}

void ArrayListSampleInstrument::SetupConstants() {

    // long
    this->ConstantLong0 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("0"), 10));
    this->ConstantLong1 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("1"), 10));

    // int
    this->ConstantInt0 = ConstantInt::get(pModule->getContext(), APInt(32, StringRef("0"), 10));
    this->ConstantInt1 = ConstantInt::get(pModule->getContext(), APInt(32, StringRef("1"), 10));
    this->ConstantSamplingRate = ConstantInt::get(pModule->getContext(),
                                                  APInt(32, StringRef(std::to_string(SamplingRate)), 10));
}

void ArrayListSampleInstrument::SetupGlobals() {
//    assert(pModule->getGlobalVariable("numCost") == NULL);
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

}

void ArrayListSampleInstrument::SetupFunctions() {

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

void ArrayListSampleInstrument::InstrumentInit(Instruction *firstInst) {

    CallInst *aprof_init_call = CallInst::Create(this->aprof_init, "", firstInst);
    aprof_init_call->setCallingConv(CallingConv::C);
    aprof_init_call->setTailCall(false);
    AttributeList int32_call_PAL;
    aprof_init_call->setAttributes(int32_call_PAL);

}

void ArrayListSampleInstrument::InstrumentReturn(Function *pFunction) {

    for (Function::iterator BI = pFunction->begin(); BI != pFunction->end(); BI++) {

        BasicBlock *BB = &*BI;

        for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {

            Instruction *Inst = &*II;

            switch (Inst->getOpcode()) {

                case Instruction::Ret: {
                    std::vector<Value *> vecParams;
                    LoadInst *bb_pLoad = new LoadInst(this->numCost, "", false, 8, Inst);
                    vecParams.push_back(bb_pLoad);
                    CallInst *void_49 = CallInst::Create(this->aprof_return, vecParams, "", Inst);
                    void_49->setCallingConv(CallingConv::C);
                    void_49->setTailCall(false);
                    AttributeList void_PAL;
                    void_49->setAttributes(void_PAL);


                    LoadInst *pLoad = new LoadInst(this->Switcher, "", false, 4, Inst);
                    BinaryOperator *int32_dec = BinaryOperator::Create(
                            Instruction::Sub, pLoad,
                            this->ConstantInt1, "dec", Inst);

                    new StoreInst(int32_dec, this->Switcher, false, 4, Inst);

                    break;
                }
            }
        }
    }

}

void ArrayListSampleInstrument::MarkFlag(Instruction *Inst) {
    MDBuilder MDHelper(this->pModule->getContext());
    Constant *InsID = ConstantInt::get(this->IntType, 1);
    SmallVector<Metadata *, 1> Vals;
    Vals.push_back(MDHelper.createConstant(InsID));
    Inst->setMetadata(
            ARRAY_LIST_INSERT,
            MDNode::get(this->pModule->getContext(), Vals)
    );
}

void ArrayListSampleInstrument::RemapInstruction(Instruction *I, ValueToValueMapTy &VMap) {
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

void ArrayListSampleInstrument::InstrumentHooks(Function *pFunction, Instruction *pInst) {

    Value *var = pInst->getOperand(0);
    DataLayout *dl = new DataLayout(this->pModule);
    Type *type_1 = var->getType()->getContainedType(0);

//    while (isa<PointerType>(type_1))
//        type_1 = type_1->getContainedType(0);

    if (type_1->isSized()) {
        ConstantInt *const_int6 = ConstantInt::get(
                this->pModule->getContext(),
                APInt(32, StringRef(std::to_string(dl->getTypeAllocSize(type_1))), 10));

        CastInst *ptr_50 = new BitCastInst(var, this->VoidPointerType,
                                           "", pInst);
        std::vector<Value *> void_51_params;
        void_51_params.push_back(ptr_50);
        void_51_params.push_back(const_int6);
        CallInst *void_51 = CallInst::Create(this->aprof_dump, void_51_params, "", pInst);
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

void ArrayListSampleInstrument::CreateIfElseIfBlock(Loop *pInnerLoop, vector<BasicBlock *> &vecAdded) {
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

    //condition2
    pLoad2 = new LoadInst(this->Switcher, "", false, 4, pTerminator);
    pCmp = new ICmpInst(pTerminator, ICmpInst::ICMP_SLE, pLoad2, this->ConstantInt0, "cmp1");
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

void ArrayListSampleInstrument::CloneInnerLoop(Loop *pLoop, vector<BasicBlock *> &vecAdd, ValueToValueMapTy &VMap) {
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

void ArrayListSampleInstrument::InstrumentCostUpdater(Loop *pLoop) {
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

void ArrayListSampleInstrument::InstrumentInnerLoop(Loop *pInnerLoop, PostDominatorTree *PDT) {
    set<BasicBlock *> setBlocksInLoop;

    for (Loop::block_iterator BB = pInnerLoop->block_begin(); BB != pInnerLoop->block_end(); BB++) {
        setBlocksInLoop.insert(*BB);
    }

    InstrumentCostUpdater(pInnerLoop);

    vector<LoadInst *> vecLoad;
    vector<Instruction *> vecIn;
    vector<Instruction *> vecOut;

    //created auxiliary basic block
    vector<BasicBlock *> vecAdd;
    CreateIfElseIfBlock(pInnerLoop, vecAdd);

    //clone loop
    ValueToValueMapTy VMap;
    CloneInnerLoop(pInnerLoop, vecAdd, VMap);

    Function *pFunc = pInnerLoop->getHeader()->getParent();


    for (Function::iterator BI = pFunc->begin(); BI != pFunc->end(); ++BI) {

        BasicBlock *BB = &*BI;

        if (BB->getName().str().find(".CPI") == std::string::npos) {
            continue;
        }

        for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {

            Instruction *Inst = &*II;

            if (hasMarkFlag(Inst)) {

                InstrumentHooks(pFunc, Inst);
            }
        }
    }

    InstrumentReturn(pFunc);

}

void ArrayListSampleInstrument::SetupInit(Module &M) {
    // all set up operation
    this->pModule = &M;
    SetupTypes();
    SetupConstants();
    SetupGlobals();
    SetupFunctions();

}

bool ArrayListSampleInstrument::runOnModule(Module &M) {

    SetupInit(M);

    Function *pFunction = searchFunctionByName(M, strFileName, strFuncName, uSrcLine);

    if (pFunction == NULL) {
        errs() << "Cannot find the input function\n";
        return false;
    }

    PostDominatorTree *PDT = &(getAnalysis<PostDominatorTreeWrapperPass>(*pFunction).getPostDomTree());
    DominatorTree *DT = &(getAnalysis<DominatorTreeWrapperPass>(*pFunction).getDomTree());

    LoopInfo &LoopInfo = getAnalysis<LoopInfoWrapperPass>(*pFunction).getLoopInfo();

    Loop *pLoop = searchLoopByLineNo(pFunction, &LoopInfo, uSrcLine);

    set<Value *> setArrayValue;

    Instruction *pInst = NULL;

    if (isArrayAccessLoop(pLoop, setArrayValue)) {
        errs() << "\nFOUND ARRAY 0 ACCESSING LOOP\n";

        set<Value *>::iterator itSetBegin = setArrayValue.begin();
        set<Value *>::iterator itSetEnd = setArrayValue.end();

        while (itSetBegin != itSetEnd) {
            (*itSetBegin)->dump();
            pInst = dyn_cast<Instruction>(*itSetBegin);
            itSetBegin++;
            MarkFlag(pInst);
        }

    } else if (isArrayAccessLoop1(pLoop, setArrayValue)) {
        errs() << "\nFOUND ARRAY 1 ACCESSING LOOP\n";

        set<Value *>::iterator itSetBegin = setArrayValue.begin();
        set<Value *>::iterator itSetEnd = setArrayValue.end();

        while (itSetBegin != itSetEnd) {
            (*itSetBegin)->dump();
            pInst = dyn_cast<Instruction>(*itSetBegin);
            itSetBegin++;
            MarkFlag(pInst);
        }
    }


    set<Value *> setLinkedValue;

    if (isLinkedListAccessLoop(pLoop, setLinkedValue)) {
        errs() << "\nFOUND Linked List ACCESSING LOOP\n";
        set<Value *>::iterator itSetBegin = setLinkedValue.begin();
        set<Value *>::iterator itSetEnd = setLinkedValue.end();

        while (itSetBegin != itSetEnd) {
            (*itSetBegin)->dump();
            pInst = dyn_cast<Instruction>(*itSetBegin);
            itSetBegin++;
            MarkFlag(pInst);
        }
    }

    if (pInst == NULL) {
        errs() << "Could not find Array or LinkedList in target Function!" << "\n";
        exit(0);
    }

    LoopSimplify(pLoop, DT);
    InstrumentInnerLoop(pLoop, PDT);

    Function *MainFunc = this->pModule->getFunction("main");
    assert(MainFunc != NULL);

    if (MainFunc) {

        Instruction *firstInst = MainFunc->getEntryBlock().getFirstNonPHI();
        InstrumentInit(firstInst);
    }

    return false;
}

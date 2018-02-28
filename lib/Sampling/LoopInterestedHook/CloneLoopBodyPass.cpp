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
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Support/raw_ostream.h"

#include "Common/Constant.h"
#include "Common/Helper.h"
#include "Common/Loop.h"
#include "Common/Search.h"

#include <map>

#include "Sampling/LoopInterestedHook/CloneLoopBodyPass.h"


using namespace llvm;
using namespace std;


static RegisterPass<CloneLoopBodyPass> X(
        "cross-iteration-instrument",
        "cross iteration instrument",
        true,
        true);


static cl::opt<unsigned> uSrcLine("noLine",
                                  cl::desc("Source Code Line Number"), cl::Optional,
                                  cl::value_desc("uSrcCodeLine"));

static cl::opt<std::string> strFileName("strFile",
                                        cl::desc("File Name"), cl::Optional,
                                        cl::value_desc("strFileName"));

static cl::opt<std::string> strFuncName("strFunc",
                                        cl::desc("Function Name"), cl::Optional,
                                        cl::value_desc("strFuncName"));

static cl::opt<std::string> strCloneLoopHeader("strCloneLoopHeader",
                                          cl::desc("Block Name for Inner Loop Header"), cl::Optional,
                                          cl::value_desc("strCloneLoopHeader"));


char CloneLoopBodyPass::ID = 0;

void CloneLoopBodyPass::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<PostDominatorTreeWrapperPass>();
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<ScalarEvolutionWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
}

CloneLoopBodyPass::CloneLoopBodyPass() : ModulePass(ID) {
    PassRegistry &Registry = *PassRegistry::getPassRegistry();
    initializeScalarEvolutionWrapperPassPass(Registry);
    initializeLoopInfoWrapperPassPass(Registry);
    initializePostDominatorTreeWrapperPassPass(Registry);
    initializeDominatorTreeWrapperPassPass(Registry);
}

void CloneLoopBodyPass::SetupTypes(Module *pModule) {
    this->VoidType = Type::getVoidTy(pModule->getContext());
    this->LongType = IntegerType::get(pModule->getContext(), 64);
    this->IntType = IntegerType::get(pModule->getContext(), 32);
    this->VoidPointerType = PointerType::get(IntegerType::get(pModule->getContext(), 8), 0);

}

void CloneLoopBodyPass::SetupConstants(Module *pModule) {
    this->ConstantInt0 = ConstantInt::get(pModule->getContext(), APInt(32, StringRef("0"), 10));
    this->ConstantInt1 = ConstantInt::get(pModule->getContext(), APInt(32, StringRef("1"), 10));
    this->ConstantInt2 = ConstantInt::get(pModule->getContext(), APInt(32, StringRef("2"), 10));

    this->ConstantLong0 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("0"), 10));
    this->ConstantLong1 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("1"), 10));
    this->ConstantLong1000 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("1000"), 10));

}

void CloneLoopBodyPass::SetupFunctions(Module *pModule) {
    vector<Type *> ArgTypes;
    this->aprof_geo = pModule->getFunction("aprof_geo");
    if (!this->aprof_geo) {
        // aprof_geo
        ArgTypes.push_back(this->IntType);
        FunctionType *AprofGeoType = FunctionType::get(this->IntType, ArgTypes, false);
        this->aprof_geo = Function::Create
                (AprofGeoType, GlobalValue::ExternalLinkage, "aprof_geo", pModule);
        this->aprof_geo->setCallingConv(CallingConv::C);
        ArgTypes.clear();
    }

    // aprof_read
    this->aprof_read = this->pM->getFunction("aprof_read");
//    assert(this->aprof_read != NULL);

    if (!this->aprof_read) {
        ArgTypes.push_back(this->VoidPointerType);
        ArgTypes.push_back(this->LongType);
//        ArgTypes.push_back(this->IntType);
        FunctionType *AprofReadType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->aprof_read = Function::Create(AprofReadType, GlobalValue::ExternalLinkage, "aprof_read", this->pM);
        this->aprof_read->setCallingConv(CallingConv::C);
        ArgTypes.clear();
    }
}

void CloneLoopBodyPass::SetupGlobals(Module *pModule) {

    assert(pModule->getGlobalVariable("CURRENT_SAMPLE") == NULL);
    this->CURRENT_SAMPLE = new GlobalVariable(*pModule, this->LongType, false, GlobalValue::ExternalLinkage, 0,
                                              "CURRENT_SAMPLE");
    this->CURRENT_SAMPLE->setAlignment(8);
    this->CURRENT_SAMPLE->setInitializer(this->ConstantLong0);

    assert(pModule->getGlobalVariable("MAX_SAMPLE") == NULL);
    this->MAX_SAMPLE = new GlobalVariable(*pModule, this->LongType, false, GlobalValue::ExternalLinkage, 0,
                                          "MAX_SAMPLE");
    this->MAX_SAMPLE->setAlignment(8);
    this->MAX_SAMPLE->setInitializer(this->ConstantLong1000);
}

void CloneLoopBodyPass::InstrumentRead(LoadInst *pLoad, Instruction *BeforeInst) {

    Value *var = pLoad->getOperand(0);

    if (var->getName().str() == "numCost") {
        return;
    }

    DataLayout *dl = new DataLayout(this->pM);
    Type *type_1 = var->getType()->getContainedType(0);

//    while (isa<PointerType>(type_1))
//        type_1 = type_1->getContainedType(0);

    if (type_1->isSized()) {
        ConstantInt *const_int6 = ConstantInt::get(
                this->pM->getContext(),
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

void CloneLoopBodyPass::RemapInstruction(Instruction *I, ValueToValueMapTy &VMap) {
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

void CloneLoopBodyPass::SplitHeader(Loop *pLoop) {
    Instruction *pFirstCall = NULL;

    for (BasicBlock::iterator II = this->pHeader->begin(); II != this->pHeader->end(); II++) {
        Instruction *Inst = &*II;

        if (isa<CallInst>(Inst) || isa<InvokeInst>(Inst)) {
            pFirstCall = Inst;
            break;
        }
    }

    if (pFirstCall == NULL) {
        pFirstCall = this->pHeader->getTerminator();
    }

    this->pOldHeader = this->pHeader->splitBasicBlock(pFirstCall, ".oldheader");

}

void CloneLoopBodyPass::InstrumentPreHeader(Loop *pLoop) {
    LoadInst *pLoad = NULL;
    StoreInst *pStore = NULL;

    //update instance counter in the preheader
    this->pPreHeader = pLoop->getLoopPreheader();

    assert(this->pPreHeader != NULL);

    new StoreInst(this->ConstantLong0, this->CURRENT_SAMPLE, false, 8, pPreHeader->getTerminator());

    this->pLoadMAX_SAMPLE = new LoadInst(this->MAX_SAMPLE, "MAX_SAMPLE", false, 8, pPreHeader->getTerminator());
}


void CloneLoopBodyPass::InstrumentNewHeader() {
    LoadInst *pLoad = new LoadInst(this->CURRENT_SAMPLE, "", false, this->pNewHeader->getTerminator());
    BinaryOperator *binOp = BinaryOperator::Create(Instruction::Add, pLoad,
                                                   this->ConstantLong1, "add", this->pNewHeader->getTerminator());
    new StoreInst(binOp, this->CURRENT_SAMPLE, false, 8, this->pNewHeader->getTerminator());

}

void CloneLoopBodyPass::UpdateHeader(set<BasicBlock *> &setCloned, ValueToValueMapTy &VMap) {
    //add condition
    LoadInst *pLoad1 = new LoadInst(this->CURRENT_SAMPLE, "", false, 8, this->pHeader->getTerminator());
    ICmpInst *pCmp = new ICmpInst(this->pHeader->getTerminator(), ICmpInst::ICMP_UGE, pLoad1, this->pLoadMAX_SAMPLE,
                                  "");
    BranchInst *pBranch = BranchInst::Create(this->pOldHeader, this->pNewHeader, pCmp);
    ReplaceInstWithInst(this->pHeader->getTerminator(), pBranch);

    MDNode *Node = MDBuilder(this->pM->getContext()).createBranchWeights(99999999, 1);
    pBranch->setMetadata(llvm::LLVMContext::MD_prof, Node);

}

void
CloneLoopBodyPass::UpdateExitBlock(Function *pFunction, set<BasicBlock *> &setExitBlocks, set<BasicBlock *> &setCloned,
                                   ValueToValueMapTy &VMap) {
    set<BasicBlock *>::iterator itSetBlockBegin = setExitBlocks.begin();
    set<BasicBlock *>::iterator itSetBlockEnd = setExitBlocks.end();

    for (; itSetBlockBegin != itSetBlockEnd; itSetBlockBegin++) {
        for (BasicBlock::iterator II = (*itSetBlockBegin)->begin(); II != (*itSetBlockBegin)->end(); II++) {
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

void CloneLoopBodyPass::AddHooksToInnerFunction(Function *pInnerFunction) {

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
                    if (BBName.find(".CPI.CPI2") != std::string::npos) {
                        if (LoadInst *pLoad = dyn_cast<LoadInst>(Inst)) {
                            InstrumentRead(pLoad, Inst);
                        }
                    }
                    break;
                }
            }
        }
    }
}


void CloneLoopBodyPass::DoClone(Function *pFunction, vector<BasicBlock *> &ToClone, ValueToValueMapTy &VMap,
                                set<BasicBlock *> &setCloned) {
    vector<BasicBlock *> BeenCloned;

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
            NewBB->setName(pCurrent->getName() + ".CPI2");
        }

        if (pCurrent->hasAddressTaken()) {
            errs() << "hasAddressTaken branch\n";
            exit(0);
        }

        for (BasicBlock::const_iterator II = pCurrent->begin(); II != pCurrent->end(); ++II) {
            Instruction *NewInst = II->clone();
            if (II->hasName()) {
                NewInst->setName(II->getName() + ".CPI2");
            }
            VMap[&*II] = NewInst;
            NewBB->getInstList().push_back(NewInst);
        }

        const TerminatorInst *TI = pCurrent->getTerminator();

        for (unsigned i = 0, e = TI->getNumSuccessors(); i != e; ++i) {
            ToClone.push_back(TI->getSuccessor(i));
        }

        setCloned.insert(pCurrent);
        BeenCloned.push_back(NewBB);
    }

    vector<BasicBlock *>::iterator itVecBegin = BeenCloned.begin();
    vector<BasicBlock *>::iterator itVecEnd = BeenCloned.end();

    for (; itVecBegin != itVecEnd; itVecBegin++) {
        for (BasicBlock::iterator II = (*itVecBegin)->begin(); II != (*itVecBegin)->end(); II++) {
            this->RemapInstruction(&*II, VMap);
        }
    }
}

void CloneLoopBodyPass::CloneLoopBody(Loop *pLoop, vector<Instruction *> &vecInst) {

//    StoreInst *pStore = NULL;
    LoopInfo *pLI;
    this->pHeader = pLoop->getHeader();
    Function *pFunction = this->pHeader->getParent();

    ValueToValueMapTy VCalleeMap;
    map<Function *, set<Instruction *> > FuncCallSiteMapping;
    set<BasicBlock *> setBlocksInLoop;

    for (Loop::block_iterator BB = pLoop->block_begin(); BB != pLoop->block_end(); BB++) {
        setBlocksInLoop.insert(*BB);
    }

//    CloneFunctionCalled(setBlocksInLoop, VCalleeMap, FuncCallSiteMapping);

    InstrumentPreHeader(pLoop);
    SplitHeader(pLoop);

    pLI = &(getAnalysis<LoopInfoWrapperPass>(*pFunction).getLoopInfo());
    pLoop = pLI->getLoopFor(pHeader);

    this->pPreHeader = pLoop->getLoopPreheader();

    vector<BasicBlock *> vecToDo;
    vecToDo.push_back(this->pOldHeader);

    ValueToValueMapTy VMap;

    VMap[pHeader] = this->pHeader;

    SmallVector<BasicBlock *, 4> ExitBlocks;
    set<BasicBlock *> setExitBlocks;
    pLoop->getExitBlocks(ExitBlocks);

    for (unsigned long i = 0; i < ExitBlocks.size(); i++) {
        VMap[ExitBlocks[i]] = ExitBlocks[i];
        setExitBlocks.insert(ExitBlocks[i]);
    }

    set<BasicBlock *> setCloned;
    DoClone(pFunction, vecToDo, VMap, setCloned);


    this->pNewHeader = cast<BasicBlock>(VMap[pOldHeader]);
    InstrumentNewHeader();
    UpdateHeader(setCloned, VMap);
    UpdateExitBlock(pFunction, setExitBlocks, setCloned, VMap);

    AddHooksToInnerFunction(pFunction);
}

bool CloneLoopBodyPass::runOnModule(Module &M) {

    this->pM = &M;
    Function *pFunction = SearchFunctionByName(M, strFuncName);

    if (pFunction == NULL) {
        errs() << "Cannot find the function containing the inner loop!\n";
        return false;
    }

    DominatorTree *DT = &(getAnalysis<DominatorTreeWrapperPass>(*pFunction).getDomTree());
    LoopInfo &pLI = getAnalysis<LoopInfoWrapperPass>(*pFunction).getLoopInfo();
    Loop *pLoop;


    BasicBlock *pHeader = SearchBlockByName(pFunction, strCloneLoopHeader);
//    BasicBlock *pHeader = SearchBlockByName(pFunction, "for.cond.CPI");

    if (pHeader == NULL) {
        errs() << "Cannot find the given loop header!\n";
        return false;
    }

    if (pLoop == NULL) {
        errs() << "Cannot find the inner loop!\n";
        return false;
    }

    pLoop = pLI.getLoopFor(pHeader);
    this->pHeader = pHeader;

    LoopSimplify(pLoop, DT);

    LoopInfo &pLI2 = getAnalysis<LoopInfoWrapperPass>(*pFunction).getLoopInfo();
    pLoop = pLI2.getLoopFor(pHeader);

    vector<Instruction *> vecInst;
    SetupTypes(&M);
    SetupConstants(&M);
    SetupGlobals(&M);
    SetupFunctions(&M);


    CloneLoopBody(pLoop, vecInst);

//    BasicBlock *pNewHeader = SearchBlockByName(pFunction, ".oldheader.CPI2");
//    pNewHeader->dump();

    return false;

}

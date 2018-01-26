#include <vector>
#include <cassert>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "Common/Constant.h"
#include "PrepareAprof/PrePareAprof.h"


using namespace llvm;
using namespace std;


static RegisterPass<PrepareAprof> X(
        "prepare-profiling",
        "prepare for profiling", true, true);


void GetAllReturnSite(Function *pFunction, set<ReturnInst *> &setRet) {

    for (Function::iterator BB = pFunction->begin(); BB != pFunction->end(); BB++) {

        if (isa<UnreachableInst>(BB->getTerminator())) {
            continue;
        }

        for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {
            if (ReturnInst *pRet = dyn_cast<ReturnInst>(II)) {

                setRet.insert(pRet);
            }
        }
    }
}

//check the return
void SplitReturnBlock(Function *pFunction) {
    set<ReturnInst *> setReturns;
    GetAllReturnSite(pFunction, setReturns);

    //errs() << setReturns.size() << "\n";
    assert(setReturns.size() == 1);

    set<ReturnInst *>::iterator itRetBegin = setReturns.begin();
    set<ReturnInst *>::iterator itRetEnd = setReturns.end();

    for (; itRetBegin != itRetEnd; itRetBegin++) {
        BasicBlock *pRetBlock = (*itRetBegin)->getParent();

        if (pRetBlock->size() > 1) {
            pRetBlock->splitBasicBlock(*itRetBegin, "CPI.return");
        }
    }
}

char PrepareAprof::ID = 0;

void PrepareAprof::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<PostDominatorTreeWrapperPass>();
    AU.addRequired<DominatorTreeWrapperPass>();
}

PrepareAprof::PrepareAprof() : ModulePass(ID) {
    PassRegistry &Registry = *PassRegistry::getPassRegistry();
    initializePostDominatorTreeWrapperPassPass(Registry);
    initializeDominatorTreeWrapperPassPass(Registry);

}

void PrepareAprof::SetupTypes(Module *pModule) {
    this->LongType = IntegerType::get(pModule->getContext(), 64);
}


void PrepareAprof::SetupConstants(Module *pModule) {
    this->ConstantLong0 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("0"), 10));
    this->ConstantLong1 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("1"), 10));
    this->ConstantLongN1 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("-1"), 10));
}

void PrepareAprof::SetupGlobals(Module *pModule) {
    assert(pModule->getGlobalVariable("Switcher") == NULL);
    this->Switcher = new GlobalVariable(*pModule, this->LongType,
                                        false, GlobalValue::ExternalLinkage,
                                        0, "Switcher");
    this->Switcher->setAlignment(8);
    this->Switcher->setInitializer(this->ConstantLong0);

}

BinaryOperator *PrepareAprof::CreateIfElseBlock(
        Function *pFunction, Module *pModule, std::vector<BasicBlock *> &vecAdd) {

    BasicBlock *pEntryBlock = NULL;
    BasicBlock *pElseBlock = NULL;

    LoadInst *pLoad1 = NULL;
    TerminatorInst *pTerminator = NULL;
    BinaryOperator *pAdd1 = NULL;
    ICmpInst *pCmp = NULL;
    BranchInst *pBranch = NULL;

    for (Function::iterator BI = pFunction->begin(); BI != pFunction->end(); BI++) {
        if (BI->getName().str() == "entry") {
            BasicBlock *BB = &*BI;
            pEntryBlock = BB;
            break;
        }
    }

    assert(pEntryBlock != NULL);
    BasicBlock *pRawEntryBlock = pEntryBlock->splitBasicBlock(pEntryBlock->begin(), "old.entry");
    pElseBlock = BasicBlock::Create(pModule->getContext(), ".else.body.CPI", pFunction, 0);

    pTerminator = pEntryBlock->getTerminator();
    pLoad1 = new LoadInst(this->Switcher, "", false, pTerminator);
    pLoad1->setAlignment(8);

    pCmp = new ICmpInst(pTerminator, ICmpInst::ICMP_EQ, pLoad1, this->ConstantLong0, "");
    pBranch = BranchInst::Create(pRawEntryBlock, pElseBlock, pCmp);
    ReplaceInstWithInst(pTerminator, pBranch);

    BranchInst::Create(pRawEntryBlock, pElseBlock);

    vecAdd.push_back(pEntryBlock);
    vecAdd.push_back(pRawEntryBlock);
    vecAdd.push_back(pElseBlock);

    return pAdd1;
}

void PrepareAprof::RemapInstruction(Instruction *I, ValueToValueMapTy &VMap) {

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

void PrepareAprof::CloneTargetFunction(
        Function *pFunction, vector<BasicBlock *> &vecAdd,
        ValueToValueMapTy &VMap) {

    BasicBlock *pEntry = vecAdd[0];
    BasicBlock *pRawEntry = vecAdd[1];
    BasicBlock *pElseBlock = vecAdd[2];

    set<ReturnInst *> setReturns;
    GetAllReturnSite(pFunction, setReturns);
    assert(setReturns.size() == 1);

    BasicBlock *pRetBlock = (*setReturns.begin())->getParent();

    VMap[pRetBlock] = pRetBlock;

    vector<BasicBlock *> ToClone;
    vector<BasicBlock *> BeenCloned;

    set<BasicBlock *> setCloned;

    ToClone.push_back(pRawEntry);

    while (ToClone.size() > 0) {
        BasicBlock *pCurrent = ToClone.back();
        ToClone.pop_back();

        WeakTrackingVH BBEntry = VMap[pCurrent];
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

        for (BasicBlock::const_iterator II = pCurrent->begin(); II != pCurrent->end(); ++II) {
            Instruction *NewInst = II->clone();

            if (II->hasName()) {
                NewInst->setName(II->getName() + ".CPI");
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


    //remap value used inside loop
    vector<BasicBlock *>::iterator itVecBegin = BeenCloned.begin();
    vector<BasicBlock *>::iterator itVecEnd = BeenCloned.end();

    for (; itVecBegin != itVecEnd; itVecBegin++) {
        for (BasicBlock::iterator II = (*itVecBegin)->begin(); II != (*itVecBegin)->end(); II++) {
            RemapInstruction(&*II, VMap);
        }
    }

    //update branch in the else block TODO!!!
    BasicBlock *pClonedEntry = pRawEntry;
    BranchInst *pBranch = dyn_cast<BranchInst>(pElseBlock->getTerminator());
    pBranch->setSuccessor(0, pClonedEntry);

    //update phi node in entry block
    for (BasicBlock::iterator II = pClonedEntry->begin(); II != pClonedEntry->end(); II++) {
        if (PHINode *pPHI = dyn_cast<PHINode>(II)) {
            for (unsigned i = 0, e = pPHI->getNumIncomingValues(); i != e; ++i) {
                if (pPHI->getIncomingBlock(i) == pEntry) {
                    pPHI->setIncomingBlock(i, pElseBlock);
                }

            }

        }
    }

    for (BasicBlock::iterator II = pRetBlock->begin(); II != pRetBlock->end(); II++) {
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

    map<Instruction *, set<Instruction *> > InstUserMapping;

    for (BasicBlock::iterator II = pRetBlock->begin(); II != pRetBlock->end(); II++) {
        Instruction *Inst = &*II;
        for (unsigned i = 0; i < II->getNumOperands(); i++) {
            if (Instruction *pInst = dyn_cast<Instruction>(II->getOperand(i))) {
                if (pInst->getParent() != pRetBlock && pInst->getParent() != pEntry) {
                    InstUserMapping[pInst].insert(Inst);
                }
            }
        }
    }

    DominatorTree *DT = &getAnalysis<DominatorTreeWrapperPass>(*pFunction).getDomTree();

    BasicBlock::iterator pBegin = pRetBlock->begin();
    map<Instruction *, set<Instruction *> >::iterator itMapBegin = InstUserMapping.begin();
    map<Instruction *, set<Instruction *> >::iterator itMapEnd = InstUserMapping.end();

    for (; itMapBegin != itMapEnd; itMapBegin++) {
        vector<BasicBlock *> vecToAdd;
        BasicBlock *pBlock = itMapBegin->first->getParent();

        for (pred_iterator PI = pred_begin(pRetBlock); PI != pred_end(pRetBlock); PI++) {
            if (setCloned.find(*PI) == setCloned.end()) {
                continue;
            }

            if (DT->dominates(pBlock, *PI)) {
                vecToAdd.push_back(*PI);
            }
        }

        PHINode *pPHI = PHINode::Create(itMapBegin->first->getType(), 2 * vecToAdd.size(), "", &*pBegin);

        vector<BasicBlock *>::iterator itBlockBegin = vecToAdd.begin();
        vector<BasicBlock *>::iterator itBlockEnd = vecToAdd.end();

        for (; itBlockBegin != itBlockEnd; itBlockBegin++) {

            pPHI->addIncoming(itMapBegin->first, *itBlockBegin);
            Instruction *Inst = cast<Instruction>(VMap[itMapBegin->first]);
            pPHI->addIncoming(Inst, Inst->getParent());
        }

        set<Instruction *>::iterator itSetBegin = itMapBegin->second.begin();
        set<Instruction *>::iterator itSetEnd = itMapBegin->second.end();

        for (; itSetBegin != itSetEnd; itSetBegin++) {
            for (unsigned i = 0; i < (*itSetBegin)->getNumOperands(); i++) {
                if ((*itSetBegin)->getOperand(i) == itMapBegin->first) {
                    (*itSetBegin)->setOperand(i, pPHI);
                }
            }
        }
    }
}


void PrepareAprof::SetupInit(Module *pModule) {
    // all set up operation
    this->pModule = pModule;
    SetupTypes(pModule);
    SetupConstants(pModule);
    SetupGlobals(pModule);
}


void PrepareAprof::AddSwitcher(Function *F) {
     SplitReturnBlock(F);
     std::vector<BasicBlock *> vecAdd;
     BinaryOperator *pAdd = CreateIfElseBlock(F, this->pModule, vecAdd);

     ValueToValueMapTy VMap;
     CloneTargetFunction(F, vecAdd, VMap);

    /* ---- alternative implementation ---- */
//    ValueToValueMapTy newVMap;
//    ValueToValueMapTy rawVMap;
//    LoadInst *pLoad1 = NULL;
//    ICmpInst *pCmp = NULL;
//
//    // clone new function
//    Function *newF = CloneFunction(F, newVMap);
//    // clone raw function
//    Function *rawF = CloneFunction(F, rawVMap);
//
//    // the prefix is used to
//    string name = newF->getName().str();
//    name = CLONE_FUNCTION_PREFIX + name;
//    newF->setName(name);
//
//    // delete all current basic blocks;
//    F->dropAllReferences();
//
//    // create all need block!
//    BasicBlock *newEntry = BasicBlock::Create(this->pModule->getContext(),
//                                              "newEntry", F, 0);
//
//    BasicBlock *label_if_then = BasicBlock::Create(this->pModule->getContext(),
//                                                   "if.then", F, 0);
//
//    BasicBlock *label_if_else = BasicBlock::Create(this->pModule->getContext(),
//                                                   "if.else", F, 0);
//
//    // load switcher
//    pLoad1 = new LoadInst(this->Switcher, "", false, newEntry);
//    pLoad1->setAlignment(8);
//
//    // create if (switcher == 0)
//    pCmp = new ICmpInst(*newEntry, ICmpInst::ICMP_EQ, pLoad1, this->ConstantLong0, "");
//    BranchInst::Create(label_if_then, label_if_else, pCmp, newEntry);
//
//    // collect all args to pass to newF and rawF.
//    vector<Value *> callArgs;
//    for (Function::arg_iterator k = F->arg_begin(), ke = F->arg_end(); k != ke; ++k)
//        callArgs.push_back(k);
//
//    // Block if.then: insert call new function in if.then
//    CallInst *oneCall = CallInst::Create(newF, callArgs,
//                                         (F->getReturnType()->isVoidTy()) ? "" : "theCall",
//                                         label_if_then);
//    oneCall->setTailCall(true);
//
//    // TODO: add call geo random a int num and store to switcher
//
//    if (F->getReturnType()->isVoidTy())
//        ReturnInst::Create(this->pModule->getContext(), label_if_then);
//    else
//        ReturnInst::Create(this->pModule->getContext(), oneCall, label_if_then);
//
//    // Block if.else: insert switcher-- and call raw function in if.else
//    LoadInst *loadInst_1 =  new LoadInst(this->Switcher, "", false, label_if_else);
//    loadInst_1->setAlignment(8);
//
//    BinaryOperator* int32_dec = BinaryOperator::Create(
//            Instruction::Add, loadInst_1,
//            this->ConstantLongN1, "dec", label_if_else);
//
//    StoreInst* void_35 = new StoreInst(
//            int32_dec, this->Switcher,
//            false, label_if_else);
//    void_35->setAlignment(4);
//
//    oneCall = CallInst::Create(
//            rawF, callArgs,
//            (F->getReturnType()->isVoidTy()) ? "" : "theCall",
//             label_if_else);
//    oneCall->setTailCall(true);
//
//    if (F->getReturnType()->isVoidTy())
//        ReturnInst::Create(this->pModule->getContext(), label_if_else);
//    else
//        ReturnInst::Create(this->pModule->getContext(), oneCall, label_if_else);

}

bool PrepareAprof::runOnModule(Module &M) {
    // setup init
    SetupInit(&M);

    std::vector<Function *> NeedClone;

    for (Module::iterator FI = M.begin(); FI != M.end(); FI++) {

        Function *F = &*FI;

        if (F->isDeclaration() || F->isIntrinsic())
            continue;

        if (F->getName().str() == "main")
            continue;

        NeedClone.push_back(F);

    }

    for (auto *Func: NeedClone) {
        AddSwitcher(Func);
    }

    return false;
}

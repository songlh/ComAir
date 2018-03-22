#include <map>
#include <set>
#include <stack>

#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"

#include "Common/Constant.h"
#include "Common/Helper.h"
#include "MarkFlagForAprof/MarkFlagForAprof.h"


using namespace std;
using namespace llvm;

static RegisterPass<MarkFlagForAprof> X(
        "mark-flags",
        "mark flags for instrumenting aprof functions",
        false, false);


static cl::opt<int> notOptimizing("not-optimize",
                                  cl::desc("Do not use optimizing read and write."),
                                  cl::init(0));

static cl::opt<int> isSampling("is-sampling",
                               cl::desc("Whether to perform sampling."),
                               cl::init(0));


/* local functions */

bool containCallInst(BasicBlock *BB) {
    for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {
        Instruction *I = &*II;

        if (isa<CallInst>(I)) {
            return true;
        }
    }

    return false;
}

int calculateBB(Function *pFunc) {
    int count = 0;

    for (Function::iterator BI = pFunc->begin(); BI != pFunc->end(); BI++) {
        count++;
    }

    return count;
}

bool containCallInstAfter(Instruction *I) {
    BasicBlock *B = I->getParent();
    BasicBlock::iterator II = B->begin();

    for (; II != B->end(); II++) {
        if (I == &*II) {
            break;
        }
    }

    for (; II != B->end(); II++) {
        if (isa<CallInst>(&*II)) {
            return true;
        }
    }

    return false;
}

bool containCallInstBefore(Instruction *I) {
    BasicBlock *B = I->getParent();

    for (BasicBlock::iterator II = B->begin(); II != B->end(); II++) {
        if (I == &*II) {
            break;
        }

        if (isa<CallInst>(&*II)) {
            return true;
        }
    }

    return false;
}

unsigned getInstIndex(Instruction *I) {
    BasicBlock *B = I->getParent();
    unsigned index = 0;

    for (BasicBlock::iterator II = B->begin(); II != B->end(); II++) {
        if (I == &*II) {
            return index;
        }

        index++;
    }

    assert(false);
}

bool containCallInstBetween(Instruction *I1, Instruction *I2) {
    assert(I1->getParent() == I2->getParent());
    assert(getInstIndex(I1) < getInstIndex(I2));
    BasicBlock *B = I1->getParent();

    bool first = false;

    for (BasicBlock::iterator II = B->begin(); II != B->end(); II++) {
        if (I1 == &*II) {
            first = true;
        } else if (I2 == &*II) {
            break;
        } else if (first && isa<CallInst>(&*II)) {
            return true;
        }

    }

    return false;
}

bool isSucc(BasicBlock *B1, BasicBlock *B2) {
    for (succ_iterator succ = succ_begin(B1); succ != succ_end(B1); succ++) {
        if (B2 == *succ) {
            return true;
        }
    }

    return false;
}

void computeReachabilityInfo(Function *F, map<BasicBlock *, map<BasicBlock *, int> > &ReachInfo) {
    ReachInfo.clear();

    for (Function::iterator BI = F->begin(); BI != F->end(); BI++) {

        BasicBlock *B = &*BI;
        map<BasicBlock *, int> mapTmp;
        ReachInfo[B] = mapTmp;

        for (Function::iterator BBI = F->begin(); BBI != F->end(); BBI++) {
            BasicBlock *BB = &*BBI;
            //ReachInfo[BB][BBB] = 0;
            if (B == BB) {
                ReachInfo[B][BB] = 1;
            } else {
                ReachInfo[B][BB] = 0;
            }
        }
    }

    for (Function::iterator BI = F->begin(); BI != F->end(); BI++) {
        BasicBlock *B = &*BI;

        for (succ_iterator succ = succ_begin(&*BI); succ != succ_end(&*BI); succ++) {
            BasicBlock *succB = *succ;
            ReachInfo[B][succB] = 1;
        }
    }

    while (true) {
        bool flag = true;

        for (Function::iterator BI = F->begin(); BI != F->end(); BI++) {
            BasicBlock *B = &*BI;

            for (Function::iterator BBI = F->begin(); BBI != F->end(); BBI++) {
                BasicBlock *BB = &*BBI;

                if (ReachInfo[B][BB] == 1) {
                    continue;
                }

                for (Function::iterator BBBI = F->begin(); BBBI != F->end(); BBBI++) {
                    BasicBlock *BBB = &*BBBI;

                    if (ReachInfo[B][BBB] == 1 && ReachInfo[BBB][BB] == 1) {
                        flag = false;
                        ReachInfo[B][BB] = 1;
                    }

                }
            }
        }

        if (flag) {
            break;
        }
    }
}

/* end local functions */

char MarkFlagForAprof::ID = 0;

void MarkFlagForAprof::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<PostDominatorTreeWrapperPass>();
    AU.addRequired<DominatorTreeWrapperPass>();

}

MarkFlagForAprof::MarkFlagForAprof() : ModulePass(ID) {
    PassRegistry &Registry = *PassRegistry::getPassRegistry();
    initializePostDominatorTreeWrapperPassPass(*PassRegistry::getPassRegistry());
    initializeDominatorTreeWrapperPassPass(Registry);
}

void MarkFlagForAprof::setupTypes() {
    this->IntType = IntegerType::get(this->pModule->getContext(), 32);
}

void MarkFlagForAprof::setupInit(Module *M) {
    this->pModule = M;
    setupTypes();
}

void MarkFlagForAprof::MarkFlag(Instruction *Inst, int Flag) {
    MDBuilder MDHelper(this->pModule->getContext());
    Constant *InsID = ConstantInt::get(this->IntType, Flag);
    SmallVector<Metadata *, 1> Vals;
    Vals.push_back(MDHelper.createConstant(InsID));
    Inst->setMetadata(
            APROF_INSERT_FLAG,
            MDNode::get(this->pModule->getContext(), Vals)
    );
}

void MarkFlagForAprof::MarkIgnoreOptimizedFlag(Function *F) {

    if (getExitBlockSize(F) != 1 || hasUnifiedUnreachableBlock(F)) {

        MDBuilder MDHelper(this->pModule->getContext());

        if (F->begin() != F->end()) {

            BasicBlock *BB = &*(F->begin());

            if (BB) {
                Instruction *Inst = &*(BB->getFirstInsertionPt());

                if (Inst) {
                    Constant *InsID = ConstantInt::get(this->IntType, 1);
                    SmallVector<Metadata *, 1> Vals;
                    Vals.push_back(MDHelper.createConstant(InsID));
                    Inst->setMetadata(IGNORE_OPTIMIZED_FLAG, MDNode::get(
                            this->pModule->getContext(), Vals));
                } else {

                    errs() << "Current Basic Block could not find an"
                            " instruction to mark bb cost flag." << "\n";
                }
            }
        }
    }
}

void MarkFlagForAprof::collectSkipStackRW(Function *F, set<Instruction *> &setSkip) {
    BasicBlock *BB = &F->getEntryBlock();
    vector<AllocaInst *> Allocas;

    for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {
        if (AllocaInst *AI = dyn_cast<AllocaInst>(II)) {
            if (isAllocaPromotable(AI)) {
                Allocas.push_back(AI);
            }
        }
    }

    if (Allocas.size() == 0) {
        return;
    }

    map<BasicBlock *, map<BasicBlock *, int> > ReachInfo;
    computeReachabilityInfo(F, ReachInfo);

    DominatorTree *DT = &getAnalysis<DominatorTreeWrapperPass>(*F).getDomTree();

    for (vector<AllocaInst *>::iterator itBegin = Allocas.begin(); itBegin != Allocas.end(); itBegin++) {
        AllocaInst *pAlloc = *itBegin;

        for (User *U1 : pAlloc->users()) {

            if (Instruction *I1 = dyn_cast<Instruction>(U1)) {
                if (isa<LoadInst>(I1) || isa<StoreInst>(I1)) {

                    for (User *U2: pAlloc->users()) {
                        if (U1 == U2) {
                            continue;
                        }

                        if (Instruction *I2 = dyn_cast<Instruction>(U2)) {
                            if (isa<LoadInst>(I2) || isa<StoreInst>(I2)) {
                                if (DT->dominates(I1->getParent(), I2->getParent())) {
                                    if (I2->getParent() == I1->getParent()) {
                                        if (getInstIndex(I2) > getInstIndex(I1)) {
                                            if (!containCallInstBetween(I1, I2)) {
                                                setSkip.insert(I2);
//                                                errs() << "1---I1" << *I1 << "; I2" << *I2 << "\n";
                                                break;
                                            }
                                        }
                                    } else if (isSucc(I1->getParent(), I2->getParent())) {
                                        if (!containCallInstAfter(I1) && !containCallInstBefore(I2)) {
                                            setSkip.insert(I2);
//                                            errs() << "2---I1" << *I1 << "; I2" << *I2 << "\n";
                                            break;
                                        }
                                    } else {
                                        bool flag = true;

                                        for (Function::iterator BI = F->begin(); BI != F->end(); BI++) {
                                            BasicBlock *BB = &*BI;

                                            if (ReachInfo[I1->getParent()][BB] == 1 &&
                                                ReachInfo[BB][I2->getParent()] == 1) {
                                                if (containCallInst(BB)) {
                                                    flag = false;
                                                    break;
                                                }
                                            }
                                        }

                                        if (flag) {
                                            setSkip.insert(I2);
//                                            errs() << "3---I1" << *I1 << "; I2" << *I2 << "\n";
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void MarkFlagForAprof::OptimizeReadWrite(Function *Func) {

    if (isSampling == 1) {

        if (!IsClonedFunc(Func)) {
            return;
        }

    } else if (IsIgnoreFunc(Func)) {

        return;
    }

    set<Instruction *> setSkip;
    collectSkipStackRW(Func, setSkip);

    for (Function::iterator BI = Func->begin(); BI != Func->end(); BI++) {

        for (BasicBlock::iterator II = BI->begin(); II != BI->end(); II++) {

            Instruction *Inst = &*II;

            if (GetInstructionID(Inst) <= 0) {
                continue;
            }

            switch (Inst->getOpcode()) {
                case Instruction::Load: {
                    if (setSkip.find(Inst) == setSkip.end()) {
                        MarkFlag(Inst, READ);
                    }

                    break;
                }
                case Instruction::Store: {
                    if (setSkip.find(Inst) == setSkip.end()) {
                        MarkFlag(Inst, WRITE);
                    }

                    break;
                }
            }
        }
    }
}

void MarkFlagForAprof::NotOptimizeReadWrite(Function *Func) {

    if (isSampling == 1) {

        if (!IsClonedFunc(Func)) {
            return;
        }

    } else if (IsIgnoreFunc(Func)) {

        return;
    }

    for (Function::iterator BI = Func->begin(); BI != Func->end(); BI++) {

        for (BasicBlock::iterator II = BI->begin(); II != BI->end(); II++) {

            Instruction *Inst = &*II;

            switch (Inst->getOpcode()) {
                case Instruction::Load: {
                    MarkFlag(Inst, READ);
                    break;
                }
                case Instruction::Store: {
                    MarkFlag(Inst, WRITE);
                    break;
                }
            }
        }
    }
}

bool MarkFlagForAprof::runOnModule(Module &M) {

    setupInit(&M);

    for (Module::iterator FI = pModule->begin(); FI != pModule->end(); FI++) {
        Function *Func = &*FI;
        MarkIgnoreOptimizedFlag(Func);

//        if (Func->getName() != "max") {
//            continue;
//        }

        if (notOptimizing == 1 || getIgnoreOptimizedFlag(Func)) {
            NotOptimizeReadWrite(Func);

        } else {
            OptimizeReadWrite(Func);
        }
    }

    return false;
}

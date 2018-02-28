#include <set>

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "IDAssigner/IDTagger.h"

using namespace llvm;

static RegisterPass<IDTagger> X("tag-id",
                                "Assign each instruction a unique ID",
                                false,
                                false);


static cl::opt<int> tagLoop("tag-loop",
                            cl::desc("Execute tag loop."),
                            cl::init(0));


std::set<Loop *> LoopSet; /*Set storing loop and subloop */

void getSubLoopSet(Loop *lp) {

    vector<Loop *> workList;
    if (lp != NULL) {
        workList.push_back(lp);
    }

    while (workList.size() != 0) {

        Loop *loop = workList.back();
        LoopSet.insert(loop);
        workList.pop_back();

        if (loop != nullptr && !loop->empty()) {

            std::vector<Loop *> &subloopVect = lp->getSubLoopsVector();
            if (subloopVect.size() != 0) {
                for (std::vector<Loop *>::const_iterator SI = subloopVect.begin(); SI != subloopVect.end(); SI++) {
                    if (*SI != NULL) {
                        if (LoopSet.find(*SI) == LoopSet.end()) {
                            workList.push_back(*SI);
                        }
                    }
                }

            }
        }
    }
}

void getLoopSet(Loop *lp) {
    if (lp != NULL && lp->getHeader() != NULL && !lp->empty()) {
        LoopSet.insert(lp);
        const std::vector<Loop *> &subloopVect = lp->getSubLoops();
        if (!subloopVect.empty()) {
            for (std::vector<Loop *>::const_iterator subli = subloopVect.begin(); subli != subloopVect.end(); subli++) {
                Loop *subloop = *subli;
                getLoopSet(subloop);
            }
        }
    }
}


#define DEBUG_TYPE "idtagger"
STATISTIC(NumInstructions, "Number of instructions");
STATISTIC(NumFunctions, "Number of functions");
STATISTIC(NumBasciBlocks, "Number of basic blocks");
STATISTIC(NumLoops, "Number of loops");

char IDTagger::ID = 0;

IDTagger::IDTagger() : ModulePass(ID) {
    PassRegistry &Registry = *PassRegistry::getPassRegistry();
    initializeLoopInfoWrapperPassPass(Registry);
}

void IDTagger::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesCFG();
    AU.addRequired<LoopInfoWrapperPass>();
}

void IDTagger::tagLoops(Module &M) {

    IntegerType *IntType = IntegerType::get(M.getContext(), 32);
    MDBuilder MDHelper(M.getContext());

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
            getSubLoopSet(loop); //including the loop itself
        }

        if (LoopSet.empty()) {
            continue;
        }

        for (Loop *loop:LoopSet) {

            if (loop == nullptr)
                continue;

            NumLoops++;
            Instruction *inst = &*(loop->getHeader()->begin());
            Constant *InsID = ConstantInt::get(IntType, NumLoops);
            SmallVector<Metadata *, 1> Vals;
            Vals.push_back(MDHelper.createConstant(InsID));
            inst->setMetadata("loop_id", MDNode::get(M.getContext(), Vals));
        }
    }

}

bool IDTagger::runOnModule(Module &M) {
    NumFunctions++;

    IntegerType *IntType = IntegerType::get(M.getContext(), 32);
    MDBuilder MDHelper(M.getContext());

    for (Module::iterator F = M.begin(); F != M.end(); F++) {
        if (F->begin() != F->end() && F->begin()->begin() != F->begin()->end()) {
            Constant *FunID = ConstantInt::get(IntType, NumFunctions);
            SmallVector<Metadata *, 1> Vals;
            Vals.push_back(MDHelper.createConstant(FunID));
            F->begin()->begin()->setMetadata("func_id", MDNode::get(M.getContext(), Vals));
            ++NumFunctions;
        }

        for (Function::iterator BB = F->begin(); BB != F->end(); BB++) {
            Constant *BBID = ConstantInt::get(IntType, NumBasciBlocks);
            SmallVector<Metadata *, 1> BB_Vals;
            BB_Vals.push_back(MDHelper.createConstant(BBID));
            BB->begin()->setMetadata("bb_id", MDNode::get(M.getContext(), BB_Vals));

            if (BB->begin() != BB->end()) {
                ++NumBasciBlocks;
            }

            for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {
                Constant *InsID = ConstantInt::get(IntType, NumInstructions);
                SmallVector<Metadata *, 1> Vals;
                Vals.push_back(MDHelper.createConstant(InsID));
                II->setMetadata("bb_id", MDNode::get(M.getContext(), BB_Vals));
                II->setMetadata("ins_id", MDNode::get(M.getContext(), Vals));
                ++NumInstructions;
            }
        }
    }

    if (tagLoop == 1) {
        tagLoops(M);
    }

    return true;
}
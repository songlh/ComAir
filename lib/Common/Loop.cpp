#include "llvm/IR/Dominators.h"
#include "llvm/IR/PredIteratorCache.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/SSAUpdater.h"
#include "llvm/Analysis/LoopInfo.h"

#include "Common/Loop.h"

using namespace llvm;
using namespace std;


bool isExitBlock(BasicBlock *BB, set<BasicBlock *> &ExitBlocks) {

    if (ExitBlocks.find(BB) != ExitBlocks.end()) {
        return true;
    }

    return false;
}


BasicBlock *RewriteLoopExitBlock(Loop *L, BasicBlock *Exit) {

    SmallVector<BasicBlock *, 8> LoopBlocks;
    for (pred_iterator I = pred_begin(Exit), E = pred_end(Exit); I != E; ++I) {
        BasicBlock *P = *I;
        if (L->contains(P)) {
            // Don't do this if the loop is exited via an indirect branch.
            if (isa<IndirectBrInst>(P->getTerminator())) assert(0);

            LoopBlocks.push_back(P);
        }
    }

    assert(!LoopBlocks.empty() && "No edges coming in from outside the loop?");
    BasicBlock *NewExitBB = 0;

    if (Exit->isLandingPad()) {
        //assert(0);
        return NULL;
    } else {
        NewExitBB = SplitBlockPredecessors(Exit, LoopBlocks, ".CI.loopexit");
    }

    return NewExitBB;
}

bool blockDominatesAnExit(BasicBlock *BB, DominatorTree &DT, set<BasicBlock *> &setExitBlocks) {
    DomTreeNode *DomNode = DT.getNode(BB);
    set<BasicBlock *>::iterator itSetBlockBegin = setExitBlocks.begin();
    set<BasicBlock *>::iterator itSetBlockEnd = setExitBlocks.end();

    for (; itSetBlockBegin != itSetBlockEnd; itSetBlockBegin++) {
        if (DT.dominates(DomNode, DT.getNode(*itSetBlockBegin))) {
            return true;
        }
    }

    return false;
}

bool ProcessInstruction(Instruction *Inst, std::set<BasicBlock *> &ExitBlocks, DominatorTree *DT, Loop *L,
                        PredIteratorCache &PredCache) {
    SmallVector<Use *, 16> UsesToRewrite;

    BasicBlock *InstBB = Inst->getParent();

    for (Value::use_iterator UI = Inst->use_begin(), E = Inst->use_end(); UI != E; ++UI) {

        User *U = UI->getUser();

        BasicBlock *UserBB = cast<Instruction>(U)->getParent();

        if (PHINode *PN = dyn_cast<PHINode>(U))

            UserBB = PN->getIncomingBlock(*UI);

        if (InstBB != UserBB && !L->contains(UserBB))
            UsesToRewrite.push_back(&*UI);
    }

    // If there are no uses outside the loop, exit with no change.
    if (UsesToRewrite.empty()) return false;

    // Invoke instructions are special in that their result value is not available
    // along their unwind edge. The code below tests to see whether DomBB dominates
    // the value, so adjust DomBB to the normal destination block, which is
    // effectively where the value is first usable.
    BasicBlock *DomBB = Inst->getParent();
    if (InvokeInst *Inv = dyn_cast<InvokeInst>(Inst))
        DomBB = Inv->getNormalDest();

    DomTreeNode *DomNode = DT->getNode(DomBB);

    SmallVector<PHINode *, 16> AddedPHIs;

    SSAUpdater SSAUpdate;
    SSAUpdate.Initialize(Inst->getType(), Inst->getName());

    // Insert the LCSSA phi's into all of the exit blocks dominated by the
    // value, and add them to the Phi's map.
    for (auto BBI = ExitBlocks.begin(); BBI != ExitBlocks.end(); ++BBI) {

        BasicBlock *ExitBB = *BBI;
        if (!DT->dominates(DomNode, DT->getNode(ExitBB))) continue;

        // If we already inserted something for this BB, don't reprocess it.
        if (SSAUpdate.HasValueForBlock(ExitBB)) continue;

        PHINode *PN = PHINode::Create(Inst->getType(), PredCache.size(ExitBB), Inst->getName() + ".lcssa",
                                      &*ExitBB->begin());

        // Add inputs from inside the loop for this PHI.
        for (BasicBlock *const *PI = PredCache.get(ExitBB).data(); *PI; ++PI) {
            PN->addIncoming(Inst, *PI);

            // If the exit block has a predecessor not within the loop, arrange for
            // the incoming value use corresponding to that predecessor to be
            // rewritten in terms of a different LCSSA PHI.
            if (!L->contains(*PI))
                UsesToRewrite.push_back(
                        &PN->getOperandUse(PN->getOperandNumForIncomingValue(PN->getNumIncomingValues() - 1)));
        }

        AddedPHIs.push_back(PN);

        // Remember that this phi makes the value alive in this block.
        SSAUpdate.AddAvailableValue(ExitBB, PN);
    }

    // Rewrite all uses outside the loop in terms of the new PHIs we just
    // inserted.
    for (unsigned i = 0, e = UsesToRewrite.size(); i != e; ++i) {
        // If this use is in an exit block, rewrite to use the newly inserted PHI.
        // This is required for correctness because SSAUpdate doesn't handle uses in
        // the same block.  It assumes the PHI we inserted is at the end of the
        // block.
        Instruction *User = cast<Instruction>(UsesToRewrite[i]->getUser());
        BasicBlock *UserBB = User->getParent();
        if (PHINode *PN = dyn_cast<PHINode>(User))
            UserBB = PN->getIncomingBlock(*UsesToRewrite[i]);

        if (isa<PHINode>(UserBB->begin()) && isExitBlock(UserBB, ExitBlocks)) {
            // Tell the VHs that the uses changed. This updates SCEV's caches.
            if (UsesToRewrite[i]->get()->hasValueHandle())
                ValueHandleBase::ValueIsRAUWd(*UsesToRewrite[i], &*(UserBB->begin()));
            UsesToRewrite[i]->set(&*(UserBB->begin()));
            continue;
        }

        // Otherwise, do full PHI insertion.
        SSAUpdate.RewriteUse(*UsesToRewrite[i]);
    }

    // Remove PHI nodes that did not have any uses rewritten.
    for (unsigned i = 0, e = AddedPHIs.size(); i != e; ++i) {
        if (AddedPHIs[i]->use_empty())
            AddedPHIs[i]->eraseFromParent();
    }

    return true;
}

void LoopSimplify(Loop *pLoop, DominatorTree *DT) {

    Function *pFunction = pLoop->getHeader()->getParent();

    //add predecessor
    BasicBlock *pPreHeader = pLoop->getLoopPreheader();

    if (pPreHeader == NULL) {
        BasicBlock *Header = pLoop->getHeader();

        // Compute the set of predecessors of the loop that are not in the loop.
        SmallVector<BasicBlock *, 8> OutsideBlocks;
        for (pred_iterator PI = pred_begin(Header), PE = pred_end(Header); PI != PE; ++PI) {
            BasicBlock *P = *PI;
            if (!pLoop->contains(P)) {
                // Coming in from outside the loop?
                // If the loop is branched to from an indirect branch, we won't
                // be able to fully transform the loop, because it prohibits
                // edge splitting.
                if (isa<IndirectBrInst>(P->getTerminator())) assert(0);
                // Keep track of it.
                OutsideBlocks.push_back(P);
            }
        }

        // Split out the loop pre-header.
        BasicBlock *PreheaderBB;
        if (!Header->isLandingPad()) {

            PreheaderBB = SplitBlockPredecessors(Header, OutsideBlocks, ".CI.preheader");

        } else {
            assert(0);
        }
    }

    SmallVector<BasicBlock *, 8> ExitBlocks;
    pLoop->getExitBlocks(ExitBlocks);

    set<BasicBlock *> ExitBlockSet(ExitBlocks.begin(), ExitBlocks.end());
    set<BasicBlock *> NewBasicBlock;
    set<BasicBlock *> OldBasicBlock;


    for (set<BasicBlock *>::iterator I = ExitBlockSet.begin(), E = ExitBlockSet.end(); I != E; ++I) {
        BasicBlock *ExitBlock = *I;
        for (pred_iterator PI = pred_begin(ExitBlock), PE = pred_end(ExitBlock); PI != PE; ++PI) {
            if (!pLoop->contains(*PI)) {
                BasicBlock *NewExit = RewriteLoopExitBlock(pLoop, ExitBlock);

                if (NewExit != NULL) {
                    NewBasicBlock.insert(NewExit);
                    OldBasicBlock.insert(ExitBlock);
                }

                break;
            }
        }
    }

    set<BasicBlock *>::iterator itSetBlockBegin = OldBasicBlock.begin();
    set<BasicBlock *>::iterator itSetBlockEnd = OldBasicBlock.end();

    for (; itSetBlockBegin != itSetBlockEnd; itSetBlockBegin++) {
        ExitBlockSet.erase(*itSetBlockBegin);
    }

    ExitBlockSet.insert(NewBasicBlock.begin(), NewBasicBlock.end());

    PredIteratorCache PredCache;

    for (Loop::block_iterator BBI = pLoop->block_begin(), BBE = pLoop->block_end(); BBI != BBE; ++BBI) {
        BasicBlock *BB = *BBI;

        if (!blockDominatesAnExit(BB, *DT, ExitBlockSet)) {
            continue;
        }

        for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
            // Reject two common cases fast: instructions with no uses (like stores)
            // and instructions with one use that is in the same block as this.
            if (I->use_empty() || (I->hasOneUse() && I->user_back()->getParent() == BB && !isa<PHINode>(I->user_back())))
                continue;

            ProcessInstruction(&*I, ExitBlockSet, DT, pLoop, PredCache);
        }
    }
}

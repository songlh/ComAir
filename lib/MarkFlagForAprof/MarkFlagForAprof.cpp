#include <map>
#include <set>
#include <stack>

#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/MDBuilder.h"

#include "Common/Constant.h"
#include "Common/Helper.h"
#include "MarkFlagForAprof/MarkFlagForAprof.h"


using namespace std;
using namespace llvm;

static RegisterPass<MarkFlagForAprof> X(
        "mark-flag-aprof", "mark flags for insert aprof functions", true, true);


/* local function */

bool IsInBasicBlock(int bb_id,
                    std::map<int, std::set<Value *>> VisitedValues,
                    Value *var) {

    if (VisitedValues.find(bb_id) != VisitedValues.end()) {
        std::set<Value *> temp_str_set = VisitedValues[bb_id];
        if (temp_str_set.find(var) != temp_str_set.end()) {
            return true;
        }
    }

    return false;

}

std::map<int, std::set<Value *>> UpdateVisitedMap(
        std::map<int, std::set<Value *>> VisitedValues,
        int bb_id, Value *var) {

    if (VisitedValues.find(bb_id) != VisitedValues.end()) {
        std::set<Value *> temp_str_set = VisitedValues[bb_id];
        if (temp_str_set.find(var) == temp_str_set.end()) {
            temp_str_set.insert(var);
            VisitedValues[bb_id] = temp_str_set;
        }
    } else {
        std::set<Value *> str_set;
        str_set.insert(var);
        VisitedValues[bb_id] = str_set;
    }

    return VisitedValues;

};

std::map<int, std::set<Value *>> UpdatePreVisitedToCur(
        std::map<int, std::set<Value *>> VisitedValues,
        int pre_bb_id, int bb_id) {

    if (VisitedValues.find(pre_bb_id) != VisitedValues.end()) {

        std::set<Value *> temp_str_set = VisitedValues[pre_bb_id];

        if (VisitedValues.find(bb_id) == VisitedValues.end()) {

            VisitedValues[bb_id] = temp_str_set;

        } else {

            std::set<Value *> temp_bb_str_set = VisitedValues[bb_id];
            temp_bb_str_set.insert(temp_str_set.begin(), temp_str_set.end());
        }
    }

    return VisitedValues;
};

bool isSuccAllSinglePredecessor(BasicBlock *BB) {

    // mat be BB is the last basic block!
    TerminatorInst *terInst = BB->getTerminator();
    if (terInst->getNumSuccessors() == 0) {
        return false;
    }

    for (succ_iterator si = succ_begin(BB); si != succ_end(BB); si++) {

        BasicBlock *sucBB = *si;

        if (!sucBB->getSinglePredecessor()) {
            return false;
        }
    }

    return true;

}

/* */

char MarkFlagForAprof::ID = 0;

void MarkFlagForAprof::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<PostDominatorTreeWrapperPass>();

}

MarkFlagForAprof::MarkFlagForAprof() : ModulePass(ID) {
    initializePostDominatorTreeWrapperPassPass(*PassRegistry::getPassRegistry());
}

void MarkFlagForAprof::setupTypes() {
    this->IntType = IntegerType::get(this->pModule->getContext(), 32);
}

void MarkFlagForAprof::setupInit(Module *M) {
    this->pModule = M;
    setupTypes();
}

void MarkFlagForAprof::markInstFlag(Instruction *Inst, int Flag) {
    MDBuilder MDHelper(this->pModule->getContext());
    Constant *InsID = ConstantInt::get(this->IntType, Flag);
    SmallVector<Metadata *, 1> Vals;
    Vals.push_back(MDHelper.createConstant(InsID));
    Inst->setMetadata(
            APROF_INSERT_FLAG,
            MDNode::get(this->pModule->getContext(), Vals)
    );
}

void MarkFlagForAprof::MarkFlag(BasicBlock *BB, int Num) {

    MDBuilder MDHelper(this->pModule->getContext());

    Instruction *Inst = &*(BB->getFirstInsertionPt());

    if (Inst) {

        Constant *InsID = ConstantInt::get(this->IntType, Num);
        SmallVector<Metadata *, 1> Vals;
        Vals.push_back(MDHelper.createConstant(InsID));
        Inst->setMetadata(BB_COST_FLAG, MDNode::get(
                this->pModule->getContext(), Vals));

    } else {

        errs() << "Current Basic Block could not find an"
                " instruction to mark bb cost flag." << "\n";
    }

}

void MarkFlagForAprof::MarkBBFlag(Function *F) {
    std::stack<BasicBlock *> VisitedStack;
    std::map<BasicBlock *, int> MarkedMap;
    std::set<BasicBlock *> VisitedSet;

    BasicBlock *firstBB = &*(F->begin());

    if (firstBB) {
        VisitedStack.push(firstBB);
        MarkedMap[firstBB] = 1;
    }

    while (!VisitedStack.empty()) {

        BasicBlock *topBB = VisitedStack.top();
        VisitedStack.pop();

        if (VisitedSet.find(topBB) == VisitedSet.end())
            VisitedSet.insert(topBB);
        else
            continue;

        // last bb
        TerminatorInst *terInst = topBB->getTerminator();
        if (terInst->getNumSuccessors() == 0) {
            continue;
        }

        int _count = 1;

        if (isSuccAllSinglePredecessor(topBB)) {

            _count = MarkedMap[topBB] + 1;
            MarkedMap.erase(topBB);
        }

        for (succ_iterator si = succ_begin(topBB); si != succ_end(topBB); si++) {

            if (VisitedSet.find(*si) != VisitedSet.end())
                continue;

            VisitedStack.push(*si);
            MarkedMap[*si] = _count;
        }
    }

    std::map<BasicBlock *, int>::iterator
            itMapBegin = MarkedMap.begin();
    std::map<BasicBlock *, int>::iterator
            itMapEnd = MarkedMap.end();

    while (itMapBegin != itMapEnd) {

        auto *itBB = itMapBegin->first;

        if (!itBB->getSinglePredecessor()) {

            std::set<BasicBlock *> preSet;
            int preCount = 0;
            bool flag = true;

            for (auto it = pred_begin(itBB), et = pred_end(itBB); it != et; ++it) {

                if (MarkedMap.find(*it) == MarkedMap.end()) {
                    flag = false;
                    break;
                }

                if (preCount == 0) {
                    preCount = MarkedMap[*it];

                } else if (preCount != MarkedMap[*it]) {
                    flag = false;
                    break;

                }

                preSet.insert(*it);

            }

            if (flag) {

                // erase preSet and update itBB's count
                for (auto it: preSet) {
                    MarkedMap.erase(it);
                }

                MarkedMap[itBB] = preCount + 1;
            }
        }

        itMapBegin++;
    }


    itMapBegin = MarkedMap.begin();
    itMapEnd = MarkedMap.end();

    while (itMapBegin != itMapEnd) {
        MarkFlag(itMapBegin->first, itMapBegin->second);
        itMapBegin++;

    }

}

bool MarkFlagForAprof::runOnModule(Module &M) {

    setupInit(&M);

    std::map<int, std::set<Value *>> VisitedValues;

    for (Module::iterator FI = pModule->begin(); FI != pModule->end(); FI++) {

        Function *Func = &*FI;

        if (IsIgnoreFunc(Func)) {
            continue;
        }

        MarkBBFlag(Func);

        VisitedValues.clear();

        for (Function::iterator BI = Func->begin(); BI != Func->end(); BI++) {

            BasicBlock *BB = &*BI;

            int bb_id = GetBasicBlockID(BB);

            BasicBlock *Pre_BB = BB->getSinglePredecessor();

            if (Pre_BB) {
                // if current bb has only one pre bb, we can update this's pre's values.
                int pre_bb_id = GetBasicBlockID(Pre_BB);
                VisitedValues = UpdatePreVisitedToCur(VisitedValues, pre_bb_id, bb_id);
            }

            for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {

                Instruction *Inst = &*II;

                switch (Inst->getOpcode()) {
                    case Instruction::Alloca: {
                        if (!IsInBasicBlock(bb_id, VisitedValues, Inst)) {
                            markInstFlag(Inst, READ);
                        }
                        VisitedValues = UpdateVisitedMap(
                                VisitedValues, bb_id, Inst);
                        break;
                    }
                    case Instruction::Call: {
                        // clear current VisitedValues.second
                        CallSite ci(Inst);
                        Function *Callee = dyn_cast<Function>(
                                ci.getCalledValue()->stripPointerCasts());

                        if (!IsIgnoreFunc(Callee)) {
                            if (VisitedValues.find(bb_id) != VisitedValues.end()) {
                                std::set<Value *> null_set;
                                null_set.clear();
                                VisitedValues[bb_id] = null_set;
                            }
                        }
                        break;
                    }
                    case Instruction::Load: {
                        Value *firstOp = Inst->getOperand(0);
                        if (!IsInBasicBlock(bb_id, VisitedValues, firstOp)) {
                            markInstFlag(Inst, READ);
                        }
                        VisitedValues = UpdateVisitedMap(
                                VisitedValues, bb_id, firstOp);
                        break;
                    }
                    case Instruction::Store: {
                        Value *secondOp = Inst->getOperand(1);
                        if (!IsInBasicBlock(bb_id, VisitedValues, secondOp)) {
                            markInstFlag(Inst, WRITE);
                        }
                        VisitedValues = UpdateVisitedMap(
                                VisitedValues, bb_id, secondOp);
                        break;
                    }
                }
            }
        }
    }

    return false;
}

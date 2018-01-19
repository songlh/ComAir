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


/* */

char MarkFlagForAprof::ID = 0;

void MarkFlagForAprof::getAnalysisUsage(AnalysisUsage &AU) const {

}

MarkFlagForAprof::MarkFlagForAprof() : ModulePass(ID) {
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

bool MarkFlagForAprof::runOnModule(Module &M) {

    setupInit(&M);

    std::map<int, std::set<Value *>> VisitedValues;

    for (Module::iterator FI = pModule->begin(); FI != pModule->end(); FI++) {

        Function *Func = &*FI;

        if (IsIgnoreFunc(Func)) {
            continue;
        }

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

#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/MDBuilder.h"

#include "Common/Constant.h"
#include "Common/Helper.h"

using namespace llvm;


int GetFunctionID(Function *F) {

    if (F->begin() == F->end()) {
        return -1;
    }

    Instruction *I = &*(F->begin()->begin());

    MDNode *Node = I->getMetadata("func_id");
    if (!Node) {
        return -1;
    }

    assert(Node->getNumOperands() == 1);
    const Metadata *MD = Node->getOperand(0);
    if (auto *MDV = dyn_cast<ValueAsMetadata>(MD)) {
        Value *V = MDV->getValue();
        ConstantInt *CI = dyn_cast<ConstantInt>(V);
        assert(CI);
        return CI->getZExtValue();
    }

    return -1;
}

int GetBasicBlockID(BasicBlock *BB) {
    if (BB->begin() == BB->end()) {
        return -1;
    }

    Instruction *I = &*(BB->begin());

    MDNode *Node = I->getMetadata("bb_id");
    if (!Node) {
        return -1;
    }

    assert(Node->getNumOperands() == 1);
    const Metadata *MD = Node->getOperand(0);
    if (auto *MDV = dyn_cast<ValueAsMetadata>(MD)) {
        Value *V = MDV->getValue();
        ConstantInt *CI = dyn_cast<ConstantInt>(V);
        assert(CI);
        return CI->getZExtValue();
    }

    return -1;
}

int GetInstructionID(Instruction *II) {

    MDNode *Node = II->getMetadata("ins_id");
    if (!Node) {
        return -1;
    }

    assert(Node->getNumOperands() == 1);
    const Metadata *MD = Node->getOperand(0);
    if (auto *MDV = dyn_cast<ValueAsMetadata>(MD)) {
        Value *V = MDV->getValue();
        ConstantInt *CI = dyn_cast<ConstantInt>(V);
        assert(CI);
        return CI->getZExtValue();
    }

    return -1;
}

int GetInstructionInsertFlag(Instruction *II) {

    MDNode *Node = II->getMetadata(APROF_INSERT_FLAG);
    if (!Node) {
        return -1;
    }

    assert(Node->getNumOperands() == 1);
    const Metadata *MD = Node->getOperand(0);
    if (auto *MDV = dyn_cast<ValueAsMetadata>(MD)) {
        Value *V = MDV->getValue();
        ConstantInt *CI = dyn_cast<ConstantInt>(V);
        assert(CI);
        return CI->getZExtValue();
    }

    return -1;
}

bool IsIgnoreFunc(Function *F) {

    if (F->getSection().str() == ".text.startup") {
        return true;
    }

    int FuncID = GetFunctionID(F);

    if (FuncID < 0) {
        return true;
    }

    return false;
}

int GetBBCostNum(BasicBlock *BB) {

    MDNode *Node;

    for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {

        Instruction *Inst = &*II;
        MDNode *Node = Inst->getMetadata(BB_COST_FLAG);
        if (!Node) {
            continue;
        }

        assert(Node->getNumOperands() == 1);
        const Metadata *MD = Node->getOperand(0);
        if (auto *MDV = dyn_cast<ValueAsMetadata>(MD)) {
            Value *V = MDV->getValue();
            ConstantInt *CI = dyn_cast<ConstantInt>(V);
            assert(CI);
            return CI->getZExtValue();
        }
    }

    return -1;

}

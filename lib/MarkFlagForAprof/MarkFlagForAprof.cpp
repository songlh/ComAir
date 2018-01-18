#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/Support/raw_ostream.h"

#include "Common/Constant.h"

#include "MarkFlagForAprof/MarkFlagForAprof.h"


using namespace std;


static RegisterPass<MarkFlagForAprof> X(
        "mark-flag-aprof", "mark flags for insert aprof functions", true, true);


/* local function */

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

bool isIgnoreFunc(Function *F) {

    if (F->getSection().str() == ".text.startup") {
        return true;
    }

    int FuncID = GetFunctionID(F);

    if (FuncID < 0) {
        return true;
    }

    return false;
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

bool MarkFlagForAprof::runOnModule(Module &M) {

    setupInit(&M);

    for (Module::iterator FI = this->pModule->begin();
         FI != this->pModule->end(); FI++) {

        Function *Func = &*FI;

        if (isIgnoreFunc(Func)) {
            continue;
        }

        PostDominatorTree *PDT = &getAnalysis<PostDominatorTreeWrapperPass>(
                *Func).getPostDomTree();

        for (Function::iterator BI = Func->begin(); BI != Func->end(); BI++) {

            errs() << "===============" << "\n";
            BasicBlock *BB = &*BI;
            BB->dump();

            DomTreeNodeBase<BasicBlock> *tree = PDT->getNode(BB);

            for (auto TI = tree->begin(); TI != tree->end(); TI++) {
                BasicBlock *BB = (*TI)->getBlock();
                BB->dump();
            }
            errs() << "===============" << "\n";
        }

    }

    return false;
}

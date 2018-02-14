#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Module.h"

#include "llvm/Support/Debug.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "MakeFunctionInline/MakeFunctionInline.h"


using namespace std;


static RegisterPass<MakeFunctionInline> X(
        "func-inline", "inline instrumented function",
        true, true);

static cl::opt<int> libInline("lib-inline",
                              cl::desc("runtime lib inline."),
                              cl::init(1));
int MAX_BB_SIZE = 100;

/* local function */

int getBasicBlockSize(Function *F) {

    int size = 0;

    for (Function::iterator BI = F->begin(); BI != F->end(); BI++) {
        size++;
    }

    return size;
}

/* end */

char MakeFunctionInline::ID = 0;


void MakeFunctionInline::getAnalysisUsage(AnalysisUsage &AU) const {

}

MakeFunctionInline::MakeFunctionInline() : ModulePass(ID) {

}

bool MakeFunctionInline::runOnModule(Module &M) {

    std::set<std::string> LibInlineFuncStr = {
            "insert_page_table", "query_page_table",
            "destroy_page_table", "rand_val"};

    std::set<std::string> BCInlineFuncStr = {
            "aprof_read", "aprof_write", "aprof_increment_rms",
            "aprof_init", "aprof_call_before", "aprof_return", "aprof_geo"
    };


    std::set<CallInst *> NeedToInline;

    for (Module::iterator FI = M.begin(); FI != M.end(); FI++) {
        Function *F = &*FI;

        if (getBasicBlockSize(F) > MAX_BB_SIZE) {
            continue;
        }

        for (Function::iterator BI = F->begin(); BI != F->end(); BI++) {

            BasicBlock *BB = &*BI;

            for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {

                Instruction *Inst = &*II;

                if (isa<CallInst>(Inst)) {

                    CallSite ci(Inst);
                    Function *Callee = dyn_cast<Function>(
                            ci.getCalledValue()->stripPointerCasts());

                    if (libInline == 1) {
                        if (Callee && LibInlineFuncStr.find(Callee->getName().str()) != LibInlineFuncStr.end()) {
                            CallInst *pCall = dyn_cast<CallInst>(Inst);
                            NeedToInline.insert(pCall);
                        }
                    } else {
                        if (Callee && BCInlineFuncStr.find(Callee->getName().str()) != BCInlineFuncStr.end()) {
                            CallInst *pCall = dyn_cast<CallInst>(Inst);
                            NeedToInline.insert(pCall);
                        }
                    }

                }
            }
        }
    }


    for (auto *pCall:NeedToInline) {

        InlineFunctionInfo IFI;
        bool Changed = InlineFunction(pCall, IFI);

        if (!Changed) {

            pCall->dump();
            errs() << "error inline!" << "\n";

        }
    }
}

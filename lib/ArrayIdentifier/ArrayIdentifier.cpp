#include <map>
#include <set>
#include <vector>

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"


#include "ArrayIdentifier/ArrayIdentifier.h"
#include "Common/ArrayLisnkedIndentifier.h"


using namespace std;

static RegisterPass<ArrayIdentifier> X("array-identifier", "report a loop accessing an array element in each iteration",
                                       true, false);

static cl::opt<unsigned> uSrcLine("noLine",
                                  cl::desc("Source Code Line Number for the Loop"), cl::Optional,
                                  cl::value_desc("uSrcCodeLine"));

static cl::opt<std::string> strFileName("strFile",
                                        cl::desc("File Name for the Loop"), cl::Optional,
                                        cl::value_desc("strFileName"));

static cl::opt<std::string> strFuncName("strFunc",
                                        cl::desc("Function Name"), cl::Optional,
                                        cl::value_desc("strFuncName"));


char ArrayIdentifier::ID = 0;

void ArrayIdentifier::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesCFG();
    AU.addRequired<LoopInfoWrapperPass>();
}

ArrayIdentifier::ArrayIdentifier() : ModulePass(ID) {
    PassRegistry &Registry = *PassRegistry::getPassRegistry();
    initializeLoopInfoWrapperPassPass(Registry);
}

bool ArrayIdentifier::runOnModule(Module &M) {
    Function *pFunction = searchFunctionByName(M, strFileName, strFuncName, uSrcLine);

    if (pFunction == NULL) {
        errs() << "Cannot find the input function\n";
        return false;
    }

    LoopInfo &LoopInfo = getAnalysis<LoopInfoWrapperPass>(*pFunction).getLoopInfo();

    Loop *pLoop = searchLoopByLineNo(pFunction, &LoopInfo, uSrcLine);

    set<Value *> setArrayValue;

    if (isArrayAccessLoop(pLoop, setArrayValue)) {
        errs() << "\nFOUND ARRAY ACCESSING LOOP\n";

        set<Value *>::iterator itSetBegin = setArrayValue.begin();
        set<Value *>::iterator itSetEnd = setArrayValue.end();

        while (itSetBegin != itSetEnd) {
            (*itSetBegin)->dump();
            itSetBegin++;
        }
    } else if (isArrayAccessLoop1(pLoop, setArrayValue)) {
        errs() << "\nFOUND ARRAY ACCESSING LOOP\n";

        set<Value *>::iterator itSetBegin = setArrayValue.begin();
        set<Value *>::iterator itSetEnd = setArrayValue.end();

        while (itSetBegin != itSetEnd) {
            (*itSetBegin)->dump();
            itSetBegin++;
        }
    }


    set<Value *> setLinkedValue;

    if (isLinkedListAccessLoop(pLoop, setLinkedValue)) {
        errs() << "\nFOUND Linked List ACCESSING LOOP\n";
    }

    errs() << setLinkedValue.size() << "\n";

    set<Value *>::iterator itSetBegin = setLinkedValue.begin();
    set<Value *>::iterator itSetEnd = setLinkedValue.end();

    while (itSetBegin != itSetEnd) {
        (*itSetBegin)->dump();
        itSetBegin++;
    }

    return false;
}

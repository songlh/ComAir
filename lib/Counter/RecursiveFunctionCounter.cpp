#include <fstream>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Module.h"


#include "Counter/RecursiveFunctionCounter.h"
#include "Common/Helper.h"


using namespace llvm;
using namespace std;


static RegisterPass<RecursiveFunctionCounter> X(
        "recursive-counter",
        "profiling algorithmic complexity", true, true);



static cl::opt<std::string> strFileName(
        "strFileName",
        cl::desc("The name of File to store system library"), cl::Optional,
        cl::value_desc("strFileName"));


char RecursiveFunctionCounter::ID = 0;


void RecursiveFunctionCounter::getAnalysisUsage(AnalysisUsage &AU) const {

}

RecursiveFunctionCounter::RecursiveFunctionCounter() : ModulePass(ID) {

}

bool RecursiveFunctionCounter::runOnModule(Module &M) {

    if (strFileName.empty()) {
        errs() << "Please set file name!" << "\n";
        exit(1);
    }

    ofstream storeFile;
    storeFile.open(strFileName, std::ofstream::out);
    storeFile << "FuncName,"
                    << "\n";

    int recursiveCall = 0;

    for (Module::iterator FI = M.begin(); FI != M.end(); FI++) {

        Function *F = &*FI;

        if (IsRecursiveCall(F)) {
            storeFile << F->getName().str() << "\n";
            recursiveCall++;
        }
    }

    errs() << "Recursive Function: " <<  to_string(recursiveCall) << "\n";
    storeFile.close();

    return false;
}

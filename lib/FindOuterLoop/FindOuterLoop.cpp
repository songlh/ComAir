#include <vector>
#include <sstream>
#include <set>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "FindOuterLoop/FindOuterLoop.h"
#include "Common/Helper.h"

using namespace std;

static RegisterPass<FindOuterLoop> X("find-outer-loop", "find root cause functions", true, true);

static cl::opt<std::string> strFileName(
        "strFileName",
        cl::desc("The name of File to read complexity function names"), cl::Optional,
        cl::value_desc("strFileName"));


char FindOuterLoop::ID = 0;

void FindOuterLoop::getAnalysisUsage(AnalysisUsage &AU) const {

}


FindOuterLoop::FindOuterLoop() : ModulePass(ID) {

}


void FindOuterLoop::SetupInit() {
    CollectFunctions();
}

Function *FindOuterLoop::getTargetFunctionName(Value *calledVal) {
    SmallPtrSet<const GlobalValue *, 3> Visited;

    Constant *c = dyn_cast<Constant>(calledVal);
    if (!c)
        return 0;

    while (true) {
        if (GlobalValue *gv = dyn_cast<GlobalValue>(c)) {

            if (!Visited.insert(gv).second)
                return 0;

            std::string alias = gv->getName();
            if (alias != "") {
                llvm::Module *currModule = this->pModule;
                GlobalValue *old_gv = gv;
                gv = currModule->getNamedValue(alias);
                if (!gv) {
                    errs() << "Function, " << alias.c_str()
                           << " alias for %s not found!\n"
                           << old_gv->getName().str().c_str() << "\n";
                }
            }

            if (Function *f = dyn_cast<Function>(gv))
                return f;

            else if (GlobalAlias *ga = dyn_cast<GlobalAlias>(gv))
                c = ga->getAliasee();
            else
                return 0;

        } else if (llvm::ConstantExpr *ce = dyn_cast<llvm::ConstantExpr>(c)) {
            if (ce->getOpcode() == Instruction::BitCast)
                c = ce->getOperand(0);
            else
                return 0;
        } else
            return 0;
    }
}


void FindOuterLoop::CollectFunctions() {

    for (Module::iterator FI = this->pModule->begin(); FI != this->pModule->end(); FI++) {
        Function *F = &*FI;

        for (Function::iterator BI = F->begin(); BI != F->end(); BI++) {

            BasicBlock *BB = &*BI;

            for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {
                this->AllFuncs.push_back(F);
                break;
            }

            break;
        }
    }
}


std::set<Function *> FindOuterLoop::FindCandidateFuncsByFuncType(FunctionType *ft) {
    std::set<Function *> candidateFuncs;

    for (std::vector<Function *>::iterator ai = this->AllFuncs.begin(), ie = this->AllFuncs.end();
         ai != ie; ++ai) {

        Function *currFunc = *ai;
        FunctionType *currType = currFunc->getFunctionType();

        if (currType == ft) {
//            errs() << "Find candidate function:" << currFunc->getName() << "\n";
            candidateFuncs.insert(currFunc);
        }
    }

    return candidateFuncs;
}


void FindOuterLoop::DumpFindFunctions(std::set<Function *> findFunc) {

    for (std::set<Function *>::iterator ai = findFunc.begin(), ie = findFunc.end(); ai != ie; ai++) {
        Function *f = *ai;
        errs() << "Candidate function name is " << f->getName() << "\n";
    }
}

void FindOuterLoop::Execute(std::map<std::string, long> FuncNameCostMap) {
    std::map<std::string, std::set<Function *>> FuncNameMap;

    // find first level callees
    for (Module::iterator FI = this->pModule->begin(); FI != this->pModule->end(); FI++) {

        Function *F = &*FI;
        string FuncName = F->getName();
        if (FuncNameCostMap.find(FuncName) != FuncNameCostMap.end()) {
            set<Function *> currentCallees;
            long CurrentMaxCost = FuncNameCostMap[FuncName];

            // get first level callees
            for (Function::iterator BI = F->begin(); BI != F->end(); BI++) {

                BasicBlock *BB = &*BI;

                for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {

                    Instruction *Inst = &*II;

                    switch (Inst->getOpcode()) {

                        case Instruction::Call: {

                            // get callee
                            CallSite cs(Inst);
                            Function *callee = cs.getCalledFunction();

                            if (callee) {
                                string calleeName = callee->getName();
                                if (FuncNameCostMap.find(calleeName) != FuncNameCostMap.end()) {
                                    if (CurrentMaxCost > FuncNameCostMap[calleeName])
                                        currentCallees.insert(callee);
                                }

                            } else {
                                Value *callPointer = cs.getCalledValue();
                                Function *targetFunc = getTargetFunctionName(callPointer);
                                Value *sv = callPointer->stripPointerCasts();
//                                StringRef fname = sv->getName();

                                if (targetFunc) {
                                    string targetFuncName = targetFunc->getName();
                                    if (FuncNameCostMap.find(targetFuncName) != FuncNameCostMap.end()) {
                                        if (CurrentMaxCost > FuncNameCostMap[targetFuncName])
                                            currentCallees.insert(callee);
                                    }


                                } else if (dyn_cast<Function>(callPointer->stripPointerCasts())) {
                                    callee = dyn_cast<Function>(callPointer->stripPointerCasts());
                                    string calleeName = callee->getName();
                                    if (FuncNameCostMap.find(calleeName) != FuncNameCostMap.end()) {
                                        if (CurrentMaxCost > FuncNameCostMap[calleeName])
                                            currentCallees.insert(callee);
                                    }

                                } else {
//                                    errs() << " Could not find callee, in function:" << FuncName << "\n";
//                                    Inst->dump();
                                    CallInst *callInst = dyn_cast<CallInst>(Inst);
                                    FunctionType *functionType = callInst->getFunctionType();
                                    std::set<Function *> findFuncs = FindCandidateFuncsByFuncType(functionType);
//                                    DumpFindFunctions(findFuncs);
                                    currentCallees.insert(findFuncs.begin(), findFuncs.end());
                                }
                            }
                        }
                    }
                }
            }

            FuncNameMap.insert(std::pair<string, set<Function *>>(FuncName, currentCallees));
        }
    }

    for (std::map<string, set<Function *>>::iterator it = FuncNameMap.begin(); it != FuncNameMap.end(); it++) {

        if (it->second.empty()) {
            errs() << "May be the outer loop function is : " << it->first << "\n";
        }

        if (it->second.size() == 1) {
            Function *itF = *it->second.begin();
            if (itF && itF->getName().str() == it->first.c_str()) {
                errs() << "May be the outer loop function is : " << it->first << "\n";
            }
        }
    }
}

template<typename Out>
void split(const std::string &s, char delim, Out result) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}


std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

bool FindOuterLoop::runOnModule(Module &M) {

    this->pModule = &M;

    SetupInit();

    map<string, long> FuncNamesCostMap;

    if (strFileName.empty()) {
        errs() << "Please set file name!" << "\n";
        exit(1);
    }

    std::ifstream input(strFileName);
    for (std::string line; getline(input, line);) {
        vector<string> tempItems = split(line, ',');
        FuncNamesCostMap.insert(pair<string, int>(tempItems[0], atol(tempItems[1].c_str())));
    }
    Execute(FuncNamesCostMap);
    return false;
}

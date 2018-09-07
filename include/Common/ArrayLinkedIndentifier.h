#ifndef COMAIR_ARRAYLISNKEDINDENTIFIER_H
#define COMAIR_ARRAYLISNKEDINDENTIFIER_H

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace std;

Function *searchFunctionByName(Module &M, string &strFileName, string &strFunctionName, unsigned uSourceCodeLine);

void searchBasicBlocksByLineNo(Function *F, unsigned uLineNo, vector<BasicBlock *> &vecBasicBlocks);

Loop *searchLoopByLineNo(Function *pFunction, LoopInfo *pLI, unsigned uLineNo);

bool isOneStarLoop(Loop *pLoop, set<BasicBlock *> &setBlocks);

bool isArrayAccessLoop1(Loop *pLoop, set<Value *> &setArrayValue);

bool isArrayAccessLoop(Loop *pLoop, set<Value *> &setArrayValue);

bool isLinkedListAccessLoop(Loop *pLoop, set<Value *> &setLinkedValue);

void findArrayIndexAndData(Instruction *Inst);

#endif //COMAIR_ARRAYLISNKEDINDENTIFIER_H

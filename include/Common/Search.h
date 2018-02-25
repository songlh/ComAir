#ifndef COMAIR_SEARCH_H
#define COMAIR_SEARCH_H

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"

using namespace llvm;
using namespace std;

BasicBlock *SearchBlockByName(Function *pFunction, string sName);

Function *SearchFunctionByName(Module &M, string strFunctionName);

Function *SearchFunctionByName(Module &M, string &strFileName, string &strFunctionName, unsigned uSourceCodeLine);

void SearchBasicBlocksByLineNo(Function *F, unsigned uLineNo, vector<BasicBlock *> &vecBasicBlocks);

Loop *SearchLoopByLineNo(Function *pFunction, LoopInfo *pLI, unsigned uLineNo);

#endif //COMAIR_SEARCH_H

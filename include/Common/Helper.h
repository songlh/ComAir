#ifndef COMAIR_HELPER_H
#define COMAIR_HELPER_H

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"

using namespace llvm;

int GetFunctionID(Function *F);

int GetBasicBlockID(BasicBlock *BB);

int GetInstructionID(Instruction *II);

int GetInstructionInsertFlag(Instruction *II);

bool getIgnoreOptimizedFlag(Function *F);

bool IsIgnoreFunc(Function *F);

bool IsClonedFunc(Function *F);

int GetBBCostNum(BasicBlock *BB);

unsigned long getExitBlockSize(Function *F);

bool hasUnifiedUnreachableBlock(Function *F);

#endif //COMAIR_HELPER_H

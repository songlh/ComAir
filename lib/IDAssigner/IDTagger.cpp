#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "IDAssigner/IDTagger.h"

using namespace llvm;

static RegisterPass<IDTagger> X("tag-id",
								"Assign each instruction a unique ID",
								false,
								false);

#define DEBUG_TYPE "idtagger"
STATISTIC(NumInstructions, "Number of instructions");
STATISTIC(NumFunctions, "Number of functions");
STATISTIC(NumBasciBlocks, "Number of basic blocks");

char IDTagger::ID = 0;

IDTagger::IDTagger(): ModulePass(ID) {}

void IDTagger::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesCFG();
}

bool IDTagger::runOnModule(Module &M)
{
	NumFunctions++;

	IntegerType * IntType = IntegerType::get(M.getContext(), 32);
	MDBuilder MDHelper(M.getContext());

	for(Module::iterator F = M.begin(); F != M.end(); F ++)
	{
		if(F->begin() != F->end() && F->begin()->begin() != F->begin()->end())
		{
			Constant * FunID = ConstantInt::get(IntType, NumFunctions);
			SmallVector<Metadata *, 1> Vals;
			Vals.push_back(MDHelper.createConstant(FunID));
			F->begin()->begin()->setMetadata("func_id", MDNode::get(M.getContext(), Vals));
			++NumFunctions;
		}

		for(Function::iterator BB = F->begin(); BB != F->end(); BB ++)
		{
			Constant * BBID = ConstantInt::get(IntType, NumBasciBlocks);
			SmallVector<Metadata *, 1> BB_Vals;
			BB_Vals.push_back(MDHelper.createConstant(BBID));
			BB->begin()->setMetadata("bb_id", MDNode::get(M.getContext(), BB_Vals));

			if(BB->begin() != BB->end())
			{
				++NumBasciBlocks;
			}

			for(BasicBlock::iterator II = BB->begin(); II != BB->end(); II ++)
			{
				Constant * InsID = ConstantInt::get(IntType, NumInstructions);
				SmallVector<Metadata *, 1> Vals;
				Vals.push_back(MDHelper.createConstant(InsID));
				II->setMetadata("bb_id", MDNode::get(M.getContext(), BB_Vals));
				II->setMetadata("ins_id", MDNode::get(M.getContext(), Vals));
				++NumInstructions;
			}
		}
	}

	return true;
}
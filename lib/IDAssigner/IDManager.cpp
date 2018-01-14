

#include "llvm/IR/Constants.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "IDAssigner/IDManager.h"

using namespace llvm;

static RegisterPass<IDManager> X("manage-id",
									"Find the instruction with a particular ID; "
									"Lookup the ID of an instruction",
									false, true);

char IDManager::ID = 0;

void IDManager::getAnalysisUsage(AnalysisUsage &AU) const 
{
	AU.setPreservesAll();
}

IDManager::IDManager(): ModulePass(ID) {}


bool IDManager::runOnModule(Module & M)
{
	InstIDMapping.clear();

	for(Module::iterator FI = M.begin(); FI != M.end(); FI++)
	{

		Function * F = &*FI;
		unsigned FuncID = getFunctionID(F);
		if(FuncID != INVALID_ID)
		{
			FuncIDMapping[FuncID].push_back(F);
		}

		for(Function::iterator BB = F->begin(); BB != F->end(); BB++)
		{
			for(BasicBlock::iterator II = BB->begin(); II != BB->end(); II ++)
			{
				Instruction * I = &*II;

				unsigned InsID = getInstructionID(I);

				if(InsID != INVALID_ID)
				{
					InstIDMapping[InsID].push_back(I);
				}

			}
		}
	}

	if(InstIDMapping_size() == 0)
	{
		errs() << "[Warning] No ID information in this program.\n";
	}

	return false;
}



unsigned IDManager::getInstructionID(Instruction * I) const
{
	MDNode * Node = I->getMetadata("ins_id");
	if(!Node)
	{
		return INVALID_ID;
	}

	assert(Node->getNumOperands() == 1);
	const Metadata * MD = Node->getOperand(0);
	if(auto *MDV = dyn_cast<ValueAsMetadata>(MD))
	{
		Value * V = MDV->getValue();
		ConstantInt * CI = dyn_cast<ConstantInt>(V);
		assert(CI);
		return CI->getZExtValue();
	}

	return INVALID_ID;

}



unsigned IDManager::getFunctionID(Function * F) const
{
	Function::iterator BI = F->begin();
	if(BI == F->end())
	{
		return INVALID_ID;
	}

	BasicBlock::iterator II = BI->begin();
	if(II == BI->end())
	{
		return INVALID_ID;
	}

	Instruction * I = &*II;

	MDNode * Node = I->getMetadata("func_id");
	if(!Node)
	{
		return INVALID_ID;
	}

	assert(Node->getNumOperands() == 1);
	const Metadata * MD = Node->getOperand(0);
	if(auto *MDV = dyn_cast<ValueAsMetadata>(MD))
	{
		Value * V = MDV->getValue();
		ConstantInt * CI = dyn_cast<ConstantInt>(V);
		assert(CI);
		return CI->getZExtValue();
	}

	return INVALID_ID;
}



Instruction *IDManager::getInstruction(unsigned InsID) const 
{
	InstList Insts = getInstructions(InsID);
	if (Insts.size() == 0 || Insts.size() > 1)
	{
		return NULL;
	}
	else
	{
		return Insts[0];
	}
}


InstList IDManager::getInstructions(unsigned InsID) const 
{
	return InstIDMapping.lookup(InsID);
}


Function * IDManager::getFunction(unsigned FuncID) const
{
	FuncList Funcs = getFunctions(FuncID);
	if(Funcs.size() == 0 || Funcs.size() > 1)
	{
		return NULL;
	}
	else
	{
		return Funcs[0];
	}
}


FuncList IDManager::getFunctions(unsigned FuncID) const
{
	return FuncIDMapping.lookup(FuncID);
}


void IDManager::print(raw_ostream &O, const Module *M) const 
{

}
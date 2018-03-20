#include <vector>
#include <map>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include "StackRWProfiling/StackRWProfiling.h"

using namespace std;

static RegisterPass<StackRWProfiling> X("stackrw-profiling", "profiling stack read and write", true, true);

char StackRWProfiling::ID = 0;

void StackRWProfiling::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequired<DominatorTreeWrapperPass>();
}

StackRWProfiling::StackRWProfiling() : ModulePass(ID) 
{
	PassRegistry &Registry = *PassRegistry::getPassRegistry();
	initializeDominatorTreeWrapperPassPass(Registry);
}

bool containCallInst(BasicBlock * BB)
{
	for(BasicBlock::iterator II = BB->begin(); II != BB->end(); II ++)
	{
		Instruction * I = &*II;

		if(isa<CallInst>(I))
		{
			return true;
		}
	}

	return false;
}


bool containCallInstAfter(Instruction * I)
{
	BasicBlock * B = I->getParent();
	BasicBlock::iterator II = B->begin();

	for(; II != B->end(); II ++)
	{
		if(I == &*II)
		{
			break;
		}
	}

	for(; II != B->end(); II ++)
	{
		if(isa<CallInst>(&*II))
		{
			return true;
		}
	}

	return false;
}


bool containCallInstBefore(Instruction * I)
{
	BasicBlock * B = I->getParent();
	
	for(BasicBlock::iterator II = B->begin(); II != B->end(); II ++ )
	{
		if(I == &* II)
		{
			break;
		}

		if(isa<CallInst>(&*II))
		{
			return true;
		}
	}

	return false;
}

bool containCallInstBetween(Instruction * I1, Instruction * I2)
{
	assert(I1->getParent() == I2->getParent());
	BasicBlock * B = I1->getParent();

	bool first = false;

	for(BasicBlock::iterator II = B->begin(); II != B->end(); II ++)
	{
		if(I1 == &* II)
		{
			first = true;
		}
		else if(I2 == &*II)
		{
			break;
		}
		else if(isa<CallInst>(&*II))
		{
			return true;
		}
		
	}

	return false;
}

unsigned getInstIndex(Instruction * I)
{
	BasicBlock * B = I->getParent();
	unsigned index = 0;

	for(BasicBlock::iterator II = B->begin(); II != B->end(); II ++)
	{
		if(I == &*II)
		{
			return index;
		}

		index ++;
	}

	assert(false);
}


bool isSucc(BasicBlock * B1, BasicBlock * B2)
{
	for(succ_iterator succ = succ_begin(B1); succ != succ_end(B1); succ ++)
	{
		if(B2 == * succ)
		{
			return true;
		}
	}

	return false;
}

void computeReachabilityInfo(Function * F, map<BasicBlock *, map<BasicBlock *, int> > & ReachInfo)
{
	ReachInfo.clear();

	for(Function::iterator BI = F->begin(); BI != F->end(); BI ++)
	{
		BasicBlock * B = &*BI;
		map<BasicBlock *, int> mapTmp;
		ReachInfo[B] = mapTmp;

		for(Function::iterator BBI = F->begin(); BBI != F->end(); BBI ++ )
		{
			BasicBlock * BB = &* BBI;
			//ReachInfo[BB][BBB] = 0;
			if(B == BB)
			{
				ReachInfo[B][BB] = 1;
 			}
 			else
 			{
 				ReachInfo[B][BB] = 0;
 			}
		}
	}

	for(Function::iterator BI = F->begin(); BI != F->end(); BI ++ )
	{
		BasicBlock * B = &* BI;

		for(succ_iterator succ = succ_begin(&*BI); succ != succ_end(&*BI); succ ++ )
		{
			BasicBlock * succB = *succ;
			ReachInfo[B][succB] = 1;
		}
	}

	while(true)
	{
		bool flag = true;

		for(Function::iterator BI = F->begin(); BI != F->end(); BI ++)
		{
			BasicBlock * B = &*BI;

			for(Function::iterator BBI = F->begin(); BBI != F->end(); BBI ++ )
			{
				BasicBlock * BB = &*BBI;

				if(ReachInfo[B][BB] == 1)
				{
					continue;
				}

				for(Function::iterator BBBI = F->begin(); BBBI != F->end(); BBBI ++ )
				{
					BasicBlock * BBB = &*BBBI;

					if(ReachInfo[B][BBB] == 1 && ReachInfo[BBB][BB] == 1)
					{
						flag = false;
						ReachInfo[B][BB] = 1;
					}
				}
			}
		}

		if(flag)
		{
			break;
		}
	}
}

void StackRWProfiling::collectSkipStackRW(Function * F, set<Instruction *> & setSkip)
{
	BasicBlock * BB = &F->getEntryBlock();
	vector<AllocaInst *> Allocas;

	for(BasicBlock::iterator II = BB->begin(); II != BB->end(); II ++)
	{
		if (AllocaInst *AI = dyn_cast<AllocaInst>(II))
		{
			if(isAllocaPromotable(AI))
			{
				Allocas.push_back(AI);
			}
		}
	}

	if(Allocas.size() == 0)
	{
		return;
	}

	map<BasicBlock *, map<BasicBlock *, int> > ReachInfo;
	computeReachabilityInfo(F, ReachInfo);

	DominatorTree * DT = &getAnalysis<DominatorTreeWrapperPass>(*F).getDomTree();

	for(vector<AllocaInst *>::iterator itBegin = Allocas.begin(); itBegin != Allocas.end(); itBegin ++ )
	{
		AllocaInst * pAlloc = *itBegin;

		for(User * U1 : pAlloc->users()) 
		{

			if(Instruction * I1 = dyn_cast<Instruction>(U1))
			{
				if(isa<LoadInst>(I1) || isa<StoreInst>(I1))
				{
					for(User * U2: pAlloc->users())
					{
						if(U1 == U2)
						{
							continue;
						}

						if(Instruction * I2 = dyn_cast<Instruction>(U2))
						{
							if(isa<LoadInst>(I2) || isa<StoreInst>(I2))
							{
								if(DT->dominates(I2->getParent(), I1->getParent()))
								{
									if(I2->getParent() == I1->getParent())
									{
										if(getInstIndex(I2) > getInstIndex(I1))
										{
											if(!containCallInstBetween(I2, I1))
											{
												setSkip.insert(I1);
												break;
											}
										}
									}
									else if(isSucc(I2->getParent(), I1->getParent()))
									{
										if(!containCallInstAfter(I2) && !containCallInstBefore(I1))
										{
											setSkip.insert(I1);
											break;
										}
									}
									else
									{
										bool flag = true;

										for(Function::iterator BI = F->begin(); BI != F->end(); BI ++ )
										{
											BasicBlock * BB = &*BI;

											if(ReachInfo[I2->getParent()][BB] == 1 && ReachInfo[BB][I1->getParent()] == 1)
											{
												if(containCallInst(BB))
												{
													flag = false;
													break;
												}
											}
										}

										if(flag)
										{
											setSkip.insert(I1);
											break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

bool StackRWProfiling::runOnFunction(Function * F)
{

	set<Instruction *> setSkip;

	collectSkipStackRW(F, setSkip);

	for(set<Instruction *>::iterator itBegin = setSkip.begin(); itBegin != setSkip.end(); itBegin ++)
	{
		Instruction * I = *itBegin;
		I->dump();
	}

	return false;
}


bool StackRWProfiling::runOnModule(Module &M) 
{
	bool changed = false;


    for (Module::iterator FI = M.begin(); FI != M.end(); FI++)
    {
    	if(FI->begin() == FI->end())
    	{
    		continue;
    	}

    	changed |= runOnFunction(&*FI);
    }

	return changed;
}
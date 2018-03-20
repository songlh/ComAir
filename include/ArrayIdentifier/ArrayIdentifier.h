
#include "llvm/Pass.h"

using namespace llvm;

struct ArrayIdentifier: public ModulePass
{

	static char ID;
	ArrayIdentifier();

	virtual void getAnalysisUsage(AnalysisUsage &AU) const;
	virtual bool runOnModule(Module& M);

};
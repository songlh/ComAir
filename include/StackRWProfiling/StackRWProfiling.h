#include <set>

#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"

using namespace llvm;
using namespace std;


struct StackRWProfiling: public ModulePass
{

	static char ID;
	StackRWProfiling();

	virtual void getAnalysisUsage(AnalysisUsage &AU) const;
	virtual bool runOnModule(Module& M);

	void collectSkipStackRW(Function * F, set<Instruction *> & setSkip);

	bool runOnFunction(Function * F);

};
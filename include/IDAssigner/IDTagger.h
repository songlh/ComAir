#ifndef _H_SONGLH_IDTAGGER
#define _H_SONGLH_IDTAGGER

#include "llvm/Pass.h"

using namespace llvm;
using namespace std;


struct IDTagger: public ModulePass
{
	static char ID;
	IDTagger();

	vector<Loop *> AllLoops;

	virtual void getAnalysisUsage(AnalysisUsage &AU) const;
	virtual bool runOnModule(Module &M);

	void tagLoops(Module &M);
};

#endif

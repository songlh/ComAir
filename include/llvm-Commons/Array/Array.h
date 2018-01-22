#ifndef _H_SONGLH_ARRAY
#define _H_SONGLH_ARRAY


#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"

#include <set>
#include <map>

using namespace llvm;
using namespace std;

namespace llvm_Commons
{


	bool BeArrayAccess(Loop * pLoop, LoadInst * pLoad, ScalarEvolution * SE, DataLayout * DL);
	bool BeArrayWrite(Loop * pLoop, StoreInst * pStore, ScalarEvolution * SE, DataLayout * DL);
	void AnalyzeArrayAccess(LoadInst * pLoad, Loop * pLoop, ScalarEvolution * SE, DataLayout * DL, vector<set<Value *> > & vecResult);

}


#endif


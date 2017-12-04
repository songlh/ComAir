#ifndef _H_SONGLH_APROF
#define _H_SONGLH_APROF

#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"


using namespace llvm;

struct AlgoProfiling : public ModulePass
{
	static char ID;
	AlgoProfiling();



	virtual void getAnalysisUsage(AnalysisUsage &AU) const;
	virtual bool runOnModule(Module& M);


	void SetupTypes(Module *);
	void SetupConstants(Module *);
	void SetupGlobals(Module *);
	void SetupHooks(Module *);

	void InstrumentCostUpdater(BasicBlock * );
	void InstrumentResultDumper(Function * );

//type
	IntegerType * LongType;

	Type * VoidType;


//function
	Function * PrintExecutionCost;


//global
	GlobalVariable * numCost;

//constant
	ConstantInt * ConstantLong0;
	ConstantInt * ConstantLong1;
};


#endif
#include <vector>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Module.h"


#include "AProf/AProf.h"

static RegisterPass<AlgoProfiling> X("algo-profiling", "profiling algorithmic complexity", true, true);

char AlgoProfiling::ID = 0;

using namespace std;

void AlgoProfiling::getAnalysisUsage(AnalysisUsage &AU) const 
{

}	

AlgoProfiling::AlgoProfiling() : ModulePass(ID)
{

}

void AlgoProfiling::SetupTypes(Module * pModule)
{
	this->VoidType = Type::getVoidTy(pModule->getContext());
	this->LongType = IntegerType::get(pModule->getContext(), 64);
}


void AlgoProfiling::SetupConstants(Module * pModule)
{
	this->ConstantLong0 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("0"), 10));
	this->ConstantLong1 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("1"), 10));
}

void AlgoProfiling::SetupGlobals(Module * pModule)
{
	assert(pModule->getGlobalVariable("numCost") == NULL);
	this->numCost = new GlobalVariable( *pModule , this->LongType, false, GlobalValue::ExternalLinkage, 0, "numCost"); 
	this->numCost->setAlignment(8);
	this->numCost->setInitializer(this->ConstantLong0);
}

void AlgoProfiling::SetupHooks(Module * pModule)
{
	vector<Type *> ArgTypes;
	ArgTypes.push_back(this->LongType);
	FunctionType * PrintExecutionCostType = FunctionType::get(this->VoidType, ArgTypes, false);

	this->PrintExecutionCost = Function::Create(PrintExecutionCostType, GlobalValue::ExternalLinkage, "PrintExecutionCost", pModule);
}

void AlgoProfiling::InstrumentCostUpdater(BasicBlock * pBlock)
{
	TerminatorInst * pTerminator = pBlock->getTerminator();
	LoadInst * pLoadnumCost = new LoadInst(this->numCost, "", false, pTerminator);
	pLoadnumCost->setAlignment(8);
	BinaryOperator * pAdd = BinaryOperator::Create(Instruction::Add, pLoadnumCost, this->ConstantLong1, "add", pTerminator);
	StoreInst * pStore = new StoreInst(pAdd, this->numCost, false, pTerminator);
	pStore->setAlignment(8);
}


void AlgoProfiling::InstrumentResultDumper(Function * pMain)
{
	LoadInst * pLoad = NULL;

	for(Function::iterator BI = pMain->begin(); BI != pMain->end(); ++BI )
	{
		for(BasicBlock::iterator II = BI->begin(); II != BI->end(); ++II)
		{
			Instruction * Ins = &*II;

			if(isa<ReturnInst>(Ins))
			{
				vector<Value *> vecParams;
				pLoad = new LoadInst(this->numCost, "", false, Ins);
				pLoad->setAlignment(8);
				vecParams.push_back(pLoad);

				CallInst * pCall = CallInst::Create(this->PrintExecutionCost, vecParams, "", Ins);
				pCall->setCallingConv(CallingConv::C);
				pCall->setTailCall(false);
				AttributeList aSet;
				pCall->setAttributes(aSet);

			}
		}
	}
}

bool AlgoProfiling::runOnModule(Module & M)
{
	SetupTypes(&M);
	SetupConstants(&M);
	SetupGlobals(&M);
	SetupHooks(&M);


	for(Module::iterator FI = M.begin(); FI != M.end(); FI ++ )
	{
		for(Function::iterator BI = FI->begin(); BI != FI->end(); BI ++ )
		{
			BasicBlock * BB = &*BI;
			InstrumentCostUpdater(BB);
		}
	}

	Function * pMain = M.getFunction("main");
	InstrumentResultDumper(pMain);


	return false;
}
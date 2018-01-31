

#include "llvm/IR/CallSite.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"


#include "Common/Constant.h"
#include "Common/Helper.h"
#include "MarkFlagForAprof/MarkFlagForAprofNoOpt.h"

using namespace std;
using namespace llvm;

static RegisterPass<MarkFlagForAprofNoOpt> X(
        "mark-flags-noopt",
        "mark flags for instrumenting aprof functions without optimizations",
        false, false);


char MarkFlagForAprofNoOpt::ID = 0;

void MarkFlagForAprofNoOpt::getAnalysisUsage(AnalysisUsage &AU) const 
{

}

MarkFlagForAprofNoOpt::MarkFlagForAprofNoOpt() : ModulePass(ID) 
{

}

void MarkFlagForAprofNoOpt::markFlag(Instruction * Inst, int Flag) 
{
	MDBuilder MDHelper(this->pModule->getContext());
    Constant *InsID = ConstantInt::get(this->IntType, Flag);
    SmallVector<Metadata *, 1> Vals;
    Vals.push_back(MDHelper.createConstant(InsID));
    Inst->setMetadata(
            APROF_INSERT_FLAG,
            MDNode::get(this->pModule->getContext(), Vals)
    );
}


bool MarkFlagForAprofNoOpt::runOnModule(Module &M) 
{
	this->pModule = &M;
	this->IntType = IntegerType::get(this->pModule->getContext(), 32);


	for (Module::iterator FI = M.begin(); FI != M.end(); FI++) 
	{
        Function *Func = &*FI;

        if(IsIgnoreFunc(Func))
        {
        	continue;
        }


        for (Function::iterator BI = Func->begin(); BI != Func->end(); BI++) 
        {
        	for(BasicBlock::iterator II = BI->begin(); II != BI->end(); II ++ )
        	{
        		Instruction * Inst = &*II;

        		switch(Inst->getOpcode())
        		{
        			case Instruction::Alloca: {
        				markFlag(Inst, READ);
        				break;
        			}
        			case Instruction::Load: {
        				markFlag(Inst, READ);
        				break;
        			}
        			case Instruction::Store: {
        				markFlag(Inst, WRITE);
        				break;
        			}

        		} 
        	}

        }



    }






}
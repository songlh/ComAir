#include <fstream>
#include <set>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Module.h"


#include "Counter/LoopRecursiveCounter.h"
#include "Common/Loop.h"
#include "Common/Helper.h"


using namespace llvm;
using namespace std;


static RegisterPass<LoopRecursiveCounter> X(
        "loop-recursive-counter",
        "profiling algorithmic complexity", true, true);


static cl::opt<std::string> strFileName(
        "strFileName",
        cl::desc("The name of File to store system library"), cl::Optional,
        cl::value_desc("strFileName"));


char LoopRecursiveCounter::ID = 0;


void LoopRecursiveCounter::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<LoopInfoWrapperPass>();

}

LoopRecursiveCounter::LoopRecursiveCounter() : ModulePass(ID) {
    PassRegistry &Registry = *PassRegistry::getPassRegistry();
    initializeLoopInfoWrapperPassPass(Registry);
}

void LoopRecursiveCounter::SetupTypes(Module *pModule) {
    this->VoidType = Type::getVoidTy(pModule->getContext());
    this->LongType = IntegerType::get(pModule->getContext(), 64);
    this->IntType = IntegerType::get(pModule->getContext(), 32);
}

void LoopRecursiveCounter::SetupFunctions(Module *pModule) {
    std::vector<Type *> ArgTypes;

    // aprof_init
    this->aprof_init = pModule->getFunction("aprof_init");

    if (!this->aprof_init) {
        FunctionType *AprofInitType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->aprof_init = Function::Create(AprofInitType, GlobalValue::ExternalLinkage, "aprof_init", pModule);
        ArgTypes.clear();
    }

    // updater
    this->updater = pModule->getFunction("updater");

    if (!this->updater) {
        ArgTypes.push_back(this->IntType);
        ArgTypes.push_back(this->IntType);
        FunctionType *AprofInitType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->updater = Function::Create(AprofInitType, GlobalValue::ExternalLinkage, "updater", pModule);
        ArgTypes.clear();
    }

    // dump

    this->dump = pModule->getFunction("dump");
    if (!this->dump) {
        FunctionType *AprofInitType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->dump = Function::Create(AprofInitType, GlobalValue::ExternalLinkage, "dump", pModule);
        ArgTypes.clear();
    }
}


void LoopRecursiveCounter::InstrumentRecursiveFunc(Function *pFunction) {
    Instruction *insertInst = &*(pFunction->getEntryBlock().getFirstInsertionPt());

    int funcId = GetFunctionID(pFunction);
    ConstantInt *const_funId = ConstantInt::get(
            this->pModule->getContext(),
            APInt(32, StringRef(std::to_string(funcId)), 10));

    ConstantInt *re_flag = ConstantInt::get(
            this->pModule->getContext(),
            APInt(32, StringRef(std::to_string(0)), 10));

    std::vector<Value *> vecParams;
    vecParams.push_back(const_funId);
    vecParams.push_back(re_flag);
    CallInst *void_49 = CallInst::Create(this->updater, vecParams, "", insertInst);
    void_49->setCallingConv(CallingConv::C);
    void_49->setTailCall(false);
    AttributeList void_PAL;
    void_49->setAttributes(void_PAL);

}

void LoopRecursiveCounter::InstrumentLoop(Loop *pLoop) {
    if (pLoop) {
        BasicBlock *pLoopHeader = pLoop->getHeader();
        if (pLoopHeader) {

            int loopId = GetLoopID(pLoop);

            ConstantInt *const_Id = ConstantInt::get(
                    this->pModule->getContext(),
                    APInt(32, StringRef(std::to_string(loopId)), 10));

            ConstantInt *loop_flag = ConstantInt::get(
                    this->pModule->getContext(),
                    APInt(32, StringRef(std::to_string(1)), 10));

            Instruction *Inst = &*pLoopHeader->getFirstInsertionPt();
            std::vector<Value *> vecParams;
            vecParams.push_back(const_Id);
            vecParams.push_back(loop_flag);
            CallInst *void_49 = CallInst::Create(this->updater, vecParams, "", Inst);
            void_49->setCallingConv(CallingConv::C);
            void_49->setTailCall(false);
            AttributeList void_PAL;
            void_49->setAttributes(void_PAL);
        }
    }
}


void LoopRecursiveCounter::InstrumentInit(Instruction *firstInst) {

    CallInst *aprof_init_call = CallInst::Create(this->aprof_init, "", firstInst);
    aprof_init_call->setCallingConv(CallingConv::C);
    aprof_init_call->setTailCall(false);
    AttributeList int32_call_PAL;
    aprof_init_call->setAttributes(int32_call_PAL);

}


void LoopRecursiveCounter::InstrumentDump(Instruction *Inst) {

    std::vector<Value *> vecParams;
    CallInst *aprof_init_call = CallInst::Create(this->dump, "", Inst);
    aprof_init_call->setCallingConv(CallingConv::C);
    aprof_init_call->setTailCall(false);
    AttributeList int32_call_PAL;
    aprof_init_call->setAttributes(int32_call_PAL);

}



bool LoopRecursiveCounter::runOnModule(Module &M) {

    this->pModule = &M;
    SetupTypes(&M);
    SetupFunctions(&M);

    if (strFileName.empty()) {
        errs() << "Please set file name!" << "\n";
        exit(1);
    }

    ofstream storeFile;
    storeFile.open(strFileName, std::ofstream::out);
    storeFile << "FuncID,"
              << "LoopID,"
              << "FuncName,"
              << "LoopOrRecursive"
              << "\n";

    std::set<Loop *> LoopSet;

    for (Module::iterator FI = M.begin(); FI != M.end(); FI++) {

        Function *F = &*FI;

        if (F->empty())
            continue;

        if (IsRecursiveCall(F)) {
            InstrumentRecursiveFunc(F);
            storeFile << std::to_string(GetFunctionID(F)) << ","
                            << "0" << "," << F->getName().str()
                            << ","<< "R" << "\n";
        }


        //clear loop set
        LoopSet.clear();

        LoopInfo &LoopInfo = getAnalysis<LoopInfoWrapperPass>(*F).getLoopInfo();

        if (LoopInfo.empty()) {
            continue;
        }

        for (auto &loop:LoopInfo) {
            //LoopSet.insert(loop);
            std::set<Loop *> tempLoopSet = getSubLoopSet(loop); //including the loop itself
            LoopSet.insert(tempLoopSet.begin(), tempLoopSet.end());
        }

        if (LoopSet.empty()) {
            continue;
        }

        for (Loop *loop:LoopSet) {

            if (loop == nullptr)
                continue;
            InstrumentLoop(loop);
            storeFile << std::to_string(GetFunctionID(F)) << ","
                      << std::to_string(GetLoopID(loop)) <<
                      "," << F->getName().str() << "," << "L" << "\n";
        }
    }

    Function *pMain = M.getFunction("main");
    assert(pMain != NULL);

    if (pMain) {

        Instruction *firstInst = pMain->getEntryBlock().getFirstNonPHI();
        InstrumentInit(firstInst);

        for (Function::iterator BI = pMain->begin(); BI != pMain->end(); BI++) {

            BasicBlock *BB = &*BI;

            for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {

                Instruction *Inst = &*II;

                switch (Inst->getOpcode()) {
                    case Instruction::Ret: {
                       InstrumentDump(Inst);
                    }
                }
            }
        }

    }

    storeFile.close();

    return false;
}

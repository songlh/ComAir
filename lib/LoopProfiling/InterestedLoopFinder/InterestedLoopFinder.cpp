#include <fstream>
#include <set>
#include <vector>

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Module.h"

#include "Common/Helper.h"
#include "Common/Loop.h"
#include "LoopProfiling/InterestedLoopFinder/InterestedLoopFinder.h"
#include "Common/ArrayLisnkedIndentifier.h"

using namespace std;


static RegisterPass<InterestedLoopFinder> X("interested-loop-finder",
                                            "used to find interested loop", true, true);


static cl::opt<std::string> strFileName(
        "strFileName",
        cl::desc("The name of File to store system library"), cl::Optional,
        cl::value_desc("strFileName"));

string funcName = "";

/* end local */

char InterestedLoopFinder::ID = 0;

void InterestedLoopFinder::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<LoopInfoWrapperPass>();
}

InterestedLoopFinder::InterestedLoopFinder() : ModulePass(ID) {
    PassRegistry &Registry = *PassRegistry::getPassRegistry();
    initializeLoopInfoWrapperPassPass(Registry);
}

void InterestedLoopFinder::SetupTypes(Module *pModule) {
    this->VoidType = Type::getVoidTy(pModule->getContext());
    this->LongType = IntegerType::get(pModule->getContext(), 64);
    this->IntType = IntegerType::get(pModule->getContext(), 32);
}

void InterestedLoopFinder::SetupConstants(Module *pModule) {
    this->ConstantLong0 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("0"), 10));
    this->ConstantLong1 = ConstantInt::get(pModule->getContext(), APInt(64, StringRef("1"), 10));
}

void InterestedLoopFinder::SetupGlobals(Module *pModule) {
    assert(pModule->getGlobalVariable("numCost") == NULL);
    this->numCost = new GlobalVariable(*pModule, this->LongType, false, GlobalValue::ExternalLinkage, 0, "numCost");
    this->numCost->setAlignment(8);
    this->numCost->setInitializer(this->ConstantLong0);
}

void InterestedLoopFinder::SetupFunctions(Module *pModule) {
    std::vector<Type *> ArgTypes;

    // aprof_init
    this->aprof_init = pModule->getFunction("aprof_init");

    if (!this->aprof_init) {
        FunctionType *AprofInitType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->aprof_init = Function::Create(AprofInitType, GlobalValue::ExternalLinkage, "aprof_init", pModule);
        ArgTypes.clear();
    }

    // aprof_init
    this->aprof_loop_in = pModule->getFunction("aprof_loop_in");

    if (!this->aprof_loop_in) {
        ArgTypes.push_back(this->IntType);
        ArgTypes.push_back(this->IntType);
        FunctionType *AprofInitType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->aprof_loop_in = Function::Create(AprofInitType, GlobalValue::ExternalLinkage, "aprof_loop_in", pModule);
        ArgTypes.clear();
    }


    // aprof_init
    this->aprof_loop_out = pModule->getFunction("aprof_loop_out");

    if (!this->aprof_loop_out) {
        ArgTypes.push_back(this->IntType);
        ArgTypes.push_back(this->IntType);
        FunctionType *AprofInitType = FunctionType::get(this->VoidType, ArgTypes, false);
        this->aprof_loop_out = Function::Create(AprofInitType, GlobalValue::ExternalLinkage, "aprof_loop_out", pModule);
        ArgTypes.clear();
    }
}

void InterestedLoopFinder::SetupInit(Module *pModule) {
    // all set up operation
    this->pModule = pModule;
    SetupTypes(pModule);
    SetupConstants(pModule);
    SetupGlobals(pModule);
    SetupFunctions(pModule);
}

void InterestedLoopFinder::InstrumentLoopIn(Loop *pLoop) {

    if (pLoop) {
        BasicBlock *pLoopHeader = pLoop->getHeader();
        if (pLoopHeader) {

            int funcId = GetFunctionID(pLoopHeader->getParent());
            int loopId = GetLoopID(pLoop);

            ConstantInt *const_funId = ConstantInt::get(
                    this->pModule->getContext(),
                    APInt(32, StringRef(std::to_string(funcId)), 10));

            ConstantInt *const_loopId = ConstantInt::get(
                    this->pModule->getContext(),
                    APInt(32, StringRef(std::to_string(loopId)), 10));

            Instruction *Inst = &*pLoopHeader->getFirstInsertionPt();
            std::vector<Value *> vecParams;
            vecParams.push_back(const_funId);
            vecParams.push_back(const_loopId);
            CallInst *void_49 = CallInst::Create(this->aprof_loop_in, vecParams, "", Inst);
            void_49->setCallingConv(CallingConv::C);
            void_49->setTailCall(false);
            AttributeList void_PAL;
            void_49->setAttributes(void_PAL);
        }
    }
}

void InterestedLoopFinder::InstrumentLoopOut(Loop *pLoop) {
    if (pLoop) {

        SmallVector<BasicBlock *, 4> ExitBlocks;
        pLoop->getExitBlocks(ExitBlocks);

        for (unsigned long i = 0; i < ExitBlocks.size(); i++) {

            if (ExitBlocks[i]) {

                BasicBlock *ExitBB = ExitBlocks[i];

                int funcId = GetFunctionID(ExitBB->getParent());
                int loopId = GetLoopID(pLoop);

                ConstantInt *const_funId = ConstantInt::get(
                        this->pModule->getContext(),
                        APInt(32, StringRef(std::to_string(funcId)), 10));

                ConstantInt *const_loopId = ConstantInt::get(
                        this->pModule->getContext(),
                        APInt(32, StringRef(std::to_string(loopId)), 10));

                Instruction *Inst = &*(ExitBB->getFirstInsertionPt());
                std::vector<Value *> vecParams;
                vecParams.push_back(const_funId);
                vecParams.push_back(const_loopId);
                CallInst *void_49 = CallInst::Create(this->aprof_loop_out, vecParams, "", Inst);
                void_49->setCallingConv(CallingConv::C);
                void_49->setTailCall(false);
                AttributeList void_PAL;
                void_49->setAttributes(void_PAL);

            }
        }
    }
}

void InterestedLoopFinder::InstrumentLoop(Loop *pLoop) {

    if (pLoop) {
        InstrumentLoopIn(pLoop);
        InstrumentLoopOut(pLoop);
    }

}

void InterestedLoopFinder::InstrumentInit(Instruction *firstInst) {

    CallInst *aprof_init_call = CallInst::Create(this->aprof_init, "", firstInst);
    aprof_init_call->setCallingConv(CallingConv::C);
    aprof_init_call->setTailCall(false);
    AttributeList int32_call_PAL;
    aprof_init_call->setAttributes(int32_call_PAL);

}

bool InterestedLoopFinder::runOnModule(Module &M) {
    // setup init
    SetupInit(&M);
    std::set<Loop *> LoopSet;

    if (strFileName.empty()) {
        errs() << "Please set file name!" << "\n";
        exit(1);
    }

    ofstream loopCounterFile;
    loopCounterFile.open(strFileName, std::ofstream::out);
    loopCounterFile << "FuncID,"
                    << "LoopID,"
                    << "FuncName,"
                    << "LoopHeader,"
                    << "SrcLine,"
                    << "ArrayLoop 0,"
                    << "ArrayLoop 1,"
                    << "LinkedListLoop"
                    << "\n";

    //identify loops, and store loop-header into HeaderSet
    for (Module::iterator FI = M.begin(); FI != M.end(); FI++) {

        Function *F = &*FI;
        if (F->empty())
            continue;

        if (!funcName.empty()) {
            if(F->getName().str() != funcName) {
                continue;
            }
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

            // Array
            set<Value *> setArrayValue;
            bool isAAL_0 = isArrayAccessLoop(loop, setArrayValue);

            if (isAAL_0) {
                errs() << "FOUND ARRAY 0 ACCESSING LOOP\n";
                set<Value *>::iterator itSetBegin = setArrayValue.begin();
                set<Value *>::iterator itSetEnd = setArrayValue.end();

                while (itSetBegin != itSetEnd) {
                    (*itSetBegin)->dump();
                    itSetBegin++;
                }

            }

            bool isAAL_1 = isArrayAccessLoop1(loop, setArrayValue);
            if (isAAL_1) {
                errs() << "FOUND ARRAY 1 ACCESSING LOOP\n";
                set<Value *>::iterator itSetBegin = setArrayValue.begin();
                set<Value *>::iterator itSetEnd = setArrayValue.end();

                while (itSetBegin != itSetEnd) {
                    (*itSetBegin)->dump();
                    itSetBegin++;
                }
            }

            // linkedList
            set<Value *> setLinkedValue;

            bool isLLAL = isLinkedListAccessLoop(loop, setLinkedValue);
            if (isLLAL) {
                errs() << "FOUND Linked List ACCESSING LOOP\n";

                set<Value *>::iterator itSetBegin = setLinkedValue.begin();
                set<Value *>::iterator itSetEnd = setLinkedValue.end();

                while (itSetBegin != itSetEnd) {
                    (*itSetBegin)->dump();
                    itSetBegin++;
                }
            }

            for (BasicBlock::iterator II = loop->getHeader()->begin(); II != loop->getHeader()->end(); II++) {

                Instruction *inst = &*II;

                const DILocation *DIL = inst->getDebugLoc();
                if (!DIL)
                    continue;

                std::string str = printSrcCodeInfo(inst);
                loopCounterFile << std::to_string(GetFunctionID(F)) << ","
                                << std::to_string(GetLoopID(loop)) << ","
                                << F->getName().str() << ","
                                << loop->getHeader()->getName().str() << ","
                                << str << ","
                                << isAAL_0 << ","
                                << isAAL_1 << ","
                                << isLLAL << "\n";

                break;
            }

        }
    }

    loopCounterFile.close();


    Function *pMain = M.getFunction("main");
    assert(pMain != NULL);

    if (pMain) {

        Instruction *firstInst = pMain->getEntryBlock().getFirstNonPHI();
        InstrumentInit(firstInst);
    }


    return false;
}

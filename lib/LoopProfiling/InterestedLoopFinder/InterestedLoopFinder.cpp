#include <fstream>
#include <set>
#include <vector>

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Module.h"

#include "Common/Helper.h"
#include "LoopProfiling/InterestedLoopFinder/InterestedLoopFinder.h"

using namespace std;


static RegisterPass<InterestedLoopFinder> X("interested-loop-finder",
                                            "used to find interested loop", true, true);


static cl::opt<std::string> strFileName(
        "strFileName",
        cl::desc("The name of File to store system library"), cl::Optional,
        cl::value_desc("strFileName"));

/* local */
std::set<Loop *> LoopSet; /*Set storing loop and subloop */


void getSubLoopSet(Loop *lp) {

    vector<Loop *> workList;
    if (lp != NULL) {
        workList.push_back(lp);
    }

    while (workList.size() != 0) {

        Loop *loop = workList.back();
        LoopSet.insert(loop);
        workList.pop_back();

        if (loop != nullptr && !loop->empty()) {

            std::vector<Loop *> &subloopVect = lp->getSubLoopsVector();
            if (subloopVect.size() != 0) {
                for (std::vector<Loop *>::const_iterator SI = subloopVect.begin(); SI != subloopVect.end(); SI++) {
                    if (*SI != NULL) {
                        if (LoopSet.find(*SI) == LoopSet.end()) {
                            workList.push_back(*SI);
                        }
                    }
                }

            }
        }
    }
}

void getLoopSet(Loop *lp) {
    if (lp != NULL && lp->getHeader() != NULL && !lp->empty()) {
        LoopSet.insert(lp);
        const std::vector<Loop *> &subloopVect = lp->getSubLoops();
        if (!subloopVect.empty()) {
            for (std::vector<Loop *>::const_iterator subli = subloopVect.begin(); subli != subloopVect.end(); subli++) {
                Loop *subloop = *subli;
                getLoopSet(subloop);
            }
        }
    }
}

bool isOneStarLoop(Loop *pLoop, set<BasicBlock *> &setBlocks) {
    BasicBlock *pHeader = pLoop->getHeader();

    if (setBlocks.find(pHeader) != setBlocks.end()) {
        return true;
    }

    set<BasicBlock *> setLoopBlocks;

    for (Loop::block_iterator BB = pLoop->block_begin(); BB != pLoop->block_end(); BB++) {
        BasicBlock *B = *BB;
        setLoopBlocks.insert(B);
    }

    vector<BasicBlock *> vecWorkList;
    set<BasicBlock *> setVisited;

    for (succ_iterator succ = succ_begin(pHeader); succ != succ_end(pHeader); succ++) {
        if (setLoopBlocks.find(*succ) != setLoopBlocks.end()) {
            vecWorkList.push_back(*succ);
            setVisited.insert(*succ);
        }
    }

    while (vecWorkList.size() > 0) {
        BasicBlock *pBlock = vecWorkList.back();
        vecWorkList.pop_back();

        if (pBlock == pHeader) {
            return false;
        }

        if (setBlocks.find(pBlock) != setBlocks.end()) {
            continue;
        }

        for (succ_iterator succ = succ_begin(pBlock); succ != succ_end(pBlock); succ++) {
            if (setLoopBlocks.find(*succ) != setLoopBlocks.end() && setVisited.find(*succ) == setVisited.end()) {
                vecWorkList.push_back(*succ);
                setVisited.insert(*succ);
            }
        }
    }

    return true;
}

bool isArrayAccessLoop1(Loop *pLoop, set<Value *> &setArrayValue) {

    setArrayValue.clear();

    map<Value *, vector<LoadInst *> > mapPArrValueAccess;
    set<BasicBlock *> setLoopBlocks;

    for (Loop::block_iterator BB = pLoop->block_begin(); BB != pLoop->block_end(); BB++) {
        BasicBlock *B = *BB;
        setLoopBlocks.insert(B);
    }

    for (Loop::block_iterator BB = pLoop->block_begin(); BB != pLoop->block_end(); BB++) {
        BasicBlock *B = *BB;

        for (BasicBlock::iterator II = B->begin(); II != B->end(); II++) {
            if (LoadInst *pLoad = dyn_cast<LoadInst>(&*II)) {
                if (GetElementPtrInst *pSource = dyn_cast<GetElementPtrInst>(pLoad->getPointerOperand())) {
                    if (setLoopBlocks.find(pSource->getParent()) == setLoopBlocks.end()) {
                        continue;
                    }

                    Value *vArrayValue = pSource->getPointerOperand();
                    mapPArrValueAccess[vArrayValue].push_back(pLoad);

//                    vArrayValue->dump();
                }
            }
        }
    }

//    errs() << "Array 1" << mapPArrValueAccess.size() << "\n";

    if (mapPArrValueAccess.size() == 0) {
        return false;
    }

    set<Value *> setPArrValue;

    map<Value *, vector<LoadInst *> >::iterator itMapBegin;
    //map<Value *, vector<LoadInst *> >::iterator itMapEnd;

    for (itMapBegin = mapPArrValueAccess.begin(); itMapBegin != mapPArrValueAccess.end(); itMapBegin++) {
        vector<LoadInst *>::iterator itVecBegin = itMapBegin->second.begin();
        vector<LoadInst *>::iterator itVecEnd = itMapBegin->second.end();

        set<BasicBlock *> setAccessBlocks;

        while (itVecBegin != itVecEnd) {
            setAccessBlocks.insert((*itVecBegin)->getParent());
            itVecBegin++;
        }

        if (isOneStarLoop(pLoop, setAccessBlocks)) {
            setPArrValue.insert(itMapBegin->first);
        }
    }

//    errs() << "Array 1" << setPArrValue.size() << "\n";

    if (setPArrValue.size() == 0) {
        return false;
    }


    set<Value *>::iterator itSetBegin = setPArrValue.begin();
    set<Value *>::iterator itSetEnd = setPArrValue.end();


    while (itSetBegin != itSetEnd) {

        set<BasicBlock *> setStoreBlocks;

        for (User *U:  (*itSetBegin)->users()) {

            if (GetElementPtrInst *pGet = dyn_cast<GetElementPtrInst>(U)) {

                if (setLoopBlocks.find(pGet->getParent()) == setLoopBlocks.end()) {
                    continue;
                }

                if (pGet->getNumOperands() == 2) {

                    Value *vOffset = pGet->getOperand(1);

                    while (CastInst *pCast = dyn_cast<CastInst>(vOffset)) {
                        vOffset = pCast->getOperand(0);
                    }

                    if (LoadInst *pLoad = dyn_cast<LoadInst>(vOffset)) {

                        for (User *UOffset : pLoad->getOperand(0)->users()) {
                            if (StoreInst *pStore = dyn_cast<StoreInst>(UOffset)) {
                                if (setLoopBlocks.find(pStore->getParent()) != setLoopBlocks.end()) {
                                    setStoreBlocks.insert(pStore->getParent());
                                }
                            }
                        }
                    }

                } else if (pGet->getNumOperands() == 3) {

                    Value *vElePtr = pGet->getOperand(0);
                    if (LoadInst *pLoad = dyn_cast<LoadInst>(vElePtr)) {
                        for (User *UOffset : pLoad->getOperand(0)->users()) {
                            if (StoreInst *pStore = dyn_cast<StoreInst>(UOffset)) {
                                if (setLoopBlocks.find(pStore->getParent()) != setLoopBlocks.end()) {
                                    setStoreBlocks.insert(pStore->getParent());
                                }
                            }
                        }
                    }

                } else {
                    continue;
                }
            }
        }

//        errs() << "Array 1 st" << setStoreBlocks.size() << "\n";

        if (isOneStarLoop(pLoop, setStoreBlocks)) {
            setArrayValue.insert(*itSetBegin);
        }

        itSetBegin++;
    }

    if (setArrayValue.size() > 0) {
        return true;
    }

    return false;
}

bool isArrayAccessLoop(Loop *pLoop, set<Value *> &setArrayValue) {

    setArrayValue.clear();

    map<Value *, vector<LoadInst *> > mapPArrValueAccess;
    set<BasicBlock *> setLoopBlocks;

    for (Loop::block_iterator BB = pLoop->block_begin(); BB != pLoop->block_end(); BB++) {
        BasicBlock *B = *BB;
        setLoopBlocks.insert(B);
    }

    for (Loop::block_iterator BB = pLoop->block_begin(); BB != pLoop->block_end(); BB++) {
        BasicBlock *B = *BB;

        for (BasicBlock::iterator II = B->begin(); II != B->end(); II++) {
            if (LoadInst *pLoad = dyn_cast<LoadInst>(&*II)) {
                if (LoadInst *pSource = dyn_cast<LoadInst>(pLoad->getPointerOperand())) {
                    if (setLoopBlocks.find(pSource->getParent()) == setLoopBlocks.end()) {
                        continue;
                    }

                    Value *vArrayValue = pSource->getPointerOperand();
                    mapPArrValueAccess[vArrayValue].push_back(pLoad);
                }
            }
        }
    }

//    errs() << "Array 0" << mapPArrValueAccess.size() << "\n";

    if (mapPArrValueAccess.size() == 0) {
        return false;
    }

    set<Value *> setPArrValue;

    map<Value *, vector<LoadInst *> >::iterator itMapBegin;
    //map<Value *, vector<LoadInst *> >::iterator itMapEnd;

    for (itMapBegin = mapPArrValueAccess.begin(); itMapBegin != mapPArrValueAccess.end(); itMapBegin++) {

        vector<LoadInst *>::iterator itVecBegin = itMapBegin->second.begin();
        vector<LoadInst *>::iterator itVecEnd = itMapBegin->second.end();

        set<BasicBlock *> setAccessBlocks;

        while (itVecBegin != itVecEnd) {
            setAccessBlocks.insert((*itVecBegin)->getParent());
            itVecBegin++;
        }

        if (isOneStarLoop(pLoop, setAccessBlocks)) {
            setPArrValue.insert(itMapBegin->first);
        }
    }

    if (setPArrValue.size() == 0) {
        return false;
    }

    set<Value *>::iterator itSetBegin = setPArrValue.begin();
    set<Value *>::iterator itSetEnd = setPArrValue.end();


    while (itSetBegin != itSetEnd) {
        set<BasicBlock *> setStoreBlocks;

        for (User *U:  (*itSetBegin)->users()) {
            if (StoreInst *pStore = dyn_cast<StoreInst>(U)) {
                if (setLoopBlocks.find(pStore->getParent()) == setLoopBlocks.end()) {
                    continue;
                }

                if (GetElementPtrInst *pGetElem = dyn_cast<GetElementPtrInst>(pStore->getValueOperand())) {
                    if (setLoopBlocks.find(pGetElem->getParent()) != setLoopBlocks.end()) {
                        if (LoadInst *pLoad = dyn_cast<LoadInst>(pGetElem->getPointerOperand())) {
                            if (pLoad->getPointerOperand() == *itSetBegin) {
                                setStoreBlocks.insert(pStore->getParent());
                            }
                        }
                    }
                }
            }
        }

        if (isOneStarLoop(pLoop, setStoreBlocks)) {
            setArrayValue.insert(*itSetBegin);
        }

        itSetBegin++;
    }

    if (setArrayValue.size() > 0) {
        return true;
    }

    return false;
}

bool isLinkedListAccessLoop(Loop *pLoop, set<Value *> &setLinkedValue) {
    map<Value *, vector<LoadInst *> > mapPLLValueAccess;
    set<BasicBlock *> setLoopBlocks;

    for (Loop::block_iterator BB = pLoop->block_begin(); BB != pLoop->block_end(); BB++) {
        BasicBlock *B = *BB;
        setLoopBlocks.insert(B);
    }

    for (Loop::block_iterator BB = pLoop->block_begin(); BB != pLoop->block_end(); BB++) {
        BasicBlock *B = *BB;

        for (BasicBlock::iterator II = B->begin(); II != B->end(); II++) {
            if (LoadInst *pLoad = dyn_cast<LoadInst>(&*II)) {
                if (GetElementPtrInst *pGet = dyn_cast<GetElementPtrInst>(pLoad->getPointerOperand())) {
                    if (ConstantInt *CI = dyn_cast<ConstantInt>(pGet->getOperand(1))) {
                        if (CI->isZero()) {
                            if (LoadInst *pSource = dyn_cast<LoadInst>(pGet->getPointerOperand())) {
                                if (setLoopBlocks.find(pSource->getParent()) == setLoopBlocks.end()) {
                                    continue;
                                }

                                Value *vLLValue = pSource->getPointerOperand();
                                mapPLLValueAccess[vLLValue].push_back(pLoad);

                            }
                        }
                    }
                }
            }
        }
    }

    //errs() << mapPLLValueAccess.size() << "\n";

    if (mapPLLValueAccess.size() == 0) {
        return false;
    }

    set<Value *> setPLLValue;

    map<Value *, vector<LoadInst *> >::iterator itMapBegin;
    //map<Value *, vector<LoadInst *> >::iterator itMapEnd;

    for (itMapBegin = mapPLLValueAccess.begin(); itMapBegin != mapPLLValueAccess.end(); itMapBegin++) {
        vector<LoadInst *>::iterator itVecBegin = itMapBegin->second.begin();
        vector<LoadInst *>::iterator itVecEnd = itMapBegin->second.end();

        set<BasicBlock *> setAccessBlocks;

        while (itVecBegin != itVecEnd) {
            setAccessBlocks.insert((*itVecBegin)->getParent());
            itVecBegin++;
        }

        if (isOneStarLoop(pLoop, setAccessBlocks)) {
            setPLLValue.insert(itMapBegin->first);
        }
    }

    //errs() << setPLLValue.size() << "\n";

    if (setPLLValue.size() == 0) {
        return false;
    }


    set<Value *>::iterator itSetBegin = setPLLValue.begin();
    set<Value *>::iterator itSetEnd = setPLLValue.end();

    while (itSetBegin != itSetEnd) {
        set<BasicBlock *> setStoreBlocks;

        for (User *U:  (*itSetBegin)->users()) {
            if (StoreInst *pStore = dyn_cast<StoreInst>(U)) {
                if (setLoopBlocks.find(pStore->getParent()) == setLoopBlocks.end()) {
                    continue;
                }

                if (LoadInst *pLoad = dyn_cast<LoadInst>(pStore->getValueOperand())) {
                    if (setLoopBlocks.find(pLoad->getParent()) != setLoopBlocks.end()) {
                        if (GetElementPtrInst *pGet = dyn_cast<GetElementPtrInst>(pLoad->getPointerOperand())) {
                            if (ConstantInt *CI = dyn_cast<ConstantInt>(pGet->getOperand(1))) {
                                if (CI->isZero()) {
                                    if (LoadInst *pLoad1 = dyn_cast<LoadInst>(pGet->getPointerOperand())) {
                                        if (pLoad1->getPointerOperand() == *itSetBegin) {
                                            setStoreBlocks.insert(pStore->getParent());
                                        }
                                    }
                                }
                            }
                        } else if (AllocaInst *pAlloc = dyn_cast<AllocaInst>(pLoad->getPointerOperand())) {
                            for (User *U2 : pAlloc->users()) {
                                if (StoreInst *pStore1 = dyn_cast<StoreInst>(U2)) {
                                    if (setLoopBlocks.find(pStore1->getParent()) == setLoopBlocks.end()) {
                                        continue;
                                    }

                                    if (LoadInst *pLoad1 = dyn_cast<LoadInst>(pStore1->getValueOperand())) {
                                        if (setLoopBlocks.find(pLoad1->getParent()) == setLoopBlocks.end()) {
                                            continue;
                                        }

                                        //pLoad1->dump();

                                        if (GetElementPtrInst *pGet = dyn_cast<GetElementPtrInst>(
                                                pLoad1->getPointerOperand())) {
                                            if (ConstantInt *CI = dyn_cast<ConstantInt>(pGet->getOperand(1))) {
                                                if (CI->isZero()) {
                                                    if (LoadInst *pLoad2 = dyn_cast<LoadInst>(
                                                            pGet->getPointerOperand())) {
                                                        if (pLoad2->getPointerOperand() == *itSetBegin) {
                                                            setStoreBlocks.insert(pStore->getParent());
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

            }
        }

        if (isOneStarLoop(pLoop, setStoreBlocks)) {
            setLinkedValue.insert(*itSetBegin);
        }

        itSetBegin++;
    }

    if (setLinkedValue.size() > 0) {
        return true;
    }

    return false;
}


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

    if (strFileName.empty()) {
        errs() << "Please set file name!" << "\n";
        exit(1);
    }

    ofstream loopCounterFile;
    loopCounterFile.open(strFileName, std::ofstream::out);
    loopCounterFile << "FuncID,"
                    << "LoopID,"
                    << "FuncName,"
                    << "ArrayLoop 0,"
                    << "ArrayLoop 1,"
                    << "LinkedListLoop"
                    << "\n";

    //identify loops, and store loop-header into HeaderSet
    for (Module::iterator FI = M.begin(); FI != M.end(); FI++) {

        Function *F = &*FI;
        if (F->empty())
            continue;

        //clear loop set
        LoopSet.clear();

        LoopInfo &LoopInfo = getAnalysis<LoopInfoWrapperPass>(*F).getLoopInfo();

        if (LoopInfo.empty()) {
            continue;
        }

        for (auto &loop:LoopInfo) {
            //LoopSet.insert(loop);
            getSubLoopSet(loop); //including the loop itself
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
//                set<Value *>::iterator itSetBegin = setArrayValue.begin();
//                set<Value *>::iterator itSetEnd = setArrayValue.end();
//
//                while (itSetBegin != itSetEnd) {
//                    (*itSetBegin)->dump();
//                    itSetBegin++;
//                }

            }

            bool isAAL_1 = isArrayAccessLoop1(loop, setArrayValue);
            if (isAAL_1) {
                errs() << "FOUND ARRAY 1 ACCESSING LOOP\n";
//                set<Value *>::iterator itSetBegin = setArrayValue.begin();
//                set<Value *>::iterator itSetEnd = setArrayValue.end();
//
//                while (itSetBegin != itSetEnd) {
//                    (*itSetBegin)->dump();
//                    itSetBegin++;
//                }
            }

            // linkedList
            set<Value *> setLinkedValue;

            bool isLLAL = isLinkedListAccessLoop(loop, setLinkedValue);
            if (isLLAL) {
                errs() << "FOUND Linked List ACCESSING LOOP\n";

//                set<Value *>::iterator itSetBegin = setLinkedValue.begin();
//                set<Value *>::iterator itSetEnd = setLinkedValue.end();
//
//                while (itSetBegin != itSetEnd) {
//                    (*itSetBegin)->dump();
//                    itSetBegin++;
//                }
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

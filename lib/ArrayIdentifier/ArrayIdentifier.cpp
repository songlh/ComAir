#include <map>
#include <set>
#include <vector>

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"


#include "ArrayIdentifier/ArrayIdentifier.h"
#include "Common/Helper.h"


using namespace std;

static RegisterPass<ArrayIdentifier> X("array-identifier", "report a loop accessing an array element in each iteration",
                                       true, false);

static cl::opt<unsigned> uSrcLine("noLine",
                                  cl::desc("Source Code Line Number for the Loop"), cl::Optional,
                                  cl::value_desc("uSrcCodeLine"));

static cl::opt<std::string> strFileName("strFile",
                                        cl::desc("File Name for the Loop"), cl::Optional,
                                        cl::value_desc("strFileName"));

static cl::opt<std::string> strFuncName("strFunc",
                                        cl::desc("Function Name"), cl::Optional,
                                        cl::value_desc("strFuncName"));


char ArrayIdentifier::ID = 0;


Function *searchFunctionByName(Module &M, string &strFileName, string &strFunctionName, unsigned uSourceCodeLine) {
    Function *pFunction = NULL;
    char pPath[400];

    for (Module::iterator FF = M.begin(), FFE = M.end(); FF != FFE; FF++) {
        Function *pF = &*FF;

        if (pF->getName().find(strFunctionName) != std::string::npos) {
            map<string, pair<unsigned int, unsigned int>> mapFunctionBound;

            for (Function::iterator BB = FF->begin(), BBE = FF->end(); BB != BBE; ++BB) {
                for (BasicBlock::iterator II = BB->begin(), IIE = BB->end(); II != IIE; ++II) {
                    Instruction *I = &*II;

                    //if(MDNode *N = I->getMetadata("dbg"))
                    const DILocation *Loc = I->getDebugLoc();
                    if (Loc != NULL) {
                        //DILocation Loc(N);
                        string sFileNameForInstruction = getFileNameForInstruction(I);
                        unsigned int uLineNoForInstruction = Loc->getLine();

                        if (mapFunctionBound.find(sFileNameForInstruction) == mapFunctionBound.end()) {
                            pair<unsigned int, unsigned int> tmpPair;
                            tmpPair.first = uLineNoForInstruction;
                            tmpPair.second = uLineNoForInstruction;
                            mapFunctionBound[sFileNameForInstruction] = tmpPair;
                        } else {
                            if (mapFunctionBound[sFileNameForInstruction].first > uLineNoForInstruction) {
                                mapFunctionBound[sFileNameForInstruction].first = uLineNoForInstruction;
                            }

                            if (mapFunctionBound[sFileNameForInstruction].second < uLineNoForInstruction) {
                                mapFunctionBound[sFileNameForInstruction].second = uLineNoForInstruction;
                            }

                        } //else
                    } //if( MDNode *N = i->getMetadata("dbg") )
                }//for(BasicBlock::iterator ...)
            } //for( Function::iterator ..)


            map<string, pair<unsigned int, unsigned int> >::iterator itMapBegin = mapFunctionBound.begin();
            map<string, pair<unsigned int, unsigned int> >::iterator itMapEnd = mapFunctionBound.end();
            while (itMapBegin != itMapEnd) {
                if (itMapBegin->first.find(strFileName) != string::npos) {
                    if (itMapBegin->second.first <= uSourceCodeLine && itMapBegin->second.second >= uSourceCodeLine) {
                        pFunction = pF;
                        break;
                    }
                }
                itMapBegin++;
            }
        } // if( f->getName().find(strFunctionName) != std::string::npos )
    } // for( Module::iterator ...)

    return pFunction;
}

void searchBasicBlocksByLineNo(Function *F, unsigned uLineNo, vector<BasicBlock *> &vecBasicBlocks) {
    for (Function::iterator BB = F->begin(); BB != F->end(); BB++) {
        for (BasicBlock::iterator II = BB->begin(); II != BB->end(); II++) {
            Instruction *I = &*II;
            const DILocation *Loc = I->getDebugLoc();

            if (Loc != NULL) {
                if (uLineNo == Loc->getLine()) {
                    vecBasicBlocks.push_back(&*BB);
                    break;
                }
            }
        }
    }
}

Loop *searchLoopByLineNo(Function *pFunction, LoopInfo *pLI, unsigned uLineNo) {
    vector<BasicBlock *> vecBasicBlocks;
    searchBasicBlocksByLineNo(pFunction, uLineNo, vecBasicBlocks);

    unsigned int uDepth = 0;
    BasicBlock *pBlock = NULL;

    for (vector<BasicBlock *>::iterator itBegin = vecBasicBlocks.begin(); itBegin != vecBasicBlocks.end(); itBegin++) {
        if (pLI->getLoopDepth(*itBegin) > uDepth) {
            uDepth = pLI->getLoopDepth(*itBegin);
            pBlock = *itBegin;
        }
    }

    if (pBlock == NULL) {
        set<Loop *> setLoop;

        for (Function::iterator BB = pFunction->begin(); BB != pFunction->end(); BB++) {
            if (pLI->getLoopDepth(&*BB) > 0) {
                setLoop.insert(pLI->getLoopFor(&*BB));
            }
        }

        if (setLoop.size() == 1) {
            return *(setLoop.begin());
        }

        return NULL;
    }

    return pLI->getLoopFor(pBlock);
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

    //errs() << mapPArrValueAccess.size() << "\n";

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

void ArrayIdentifier::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesCFG();
    AU.addRequired<LoopInfoWrapperPass>();
}

ArrayIdentifier::ArrayIdentifier() : ModulePass(ID) {
    PassRegistry &Registry = *PassRegistry::getPassRegistry();
    initializeLoopInfoWrapperPassPass(Registry);
}

bool ArrayIdentifier::runOnModule(Module &M) {
    Function *pFunction = searchFunctionByName(M, strFileName, strFuncName, uSrcLine);

    if (pFunction == NULL) {
        errs() << "Cannot find the input function\n";
        return false;
    }

    LoopInfo &LoopInfo = getAnalysis<LoopInfoWrapperPass>(*pFunction).getLoopInfo();

    Loop *pLoop = searchLoopByLineNo(pFunction, &LoopInfo, uSrcLine);

    set<Value *> setArrayValue;

    if (isArrayAccessLoop(pLoop, setArrayValue)) {
        errs() << "\nFOUND ARRAY ACCESSING LOOP\n";

        set<Value *>::iterator itSetBegin = setArrayValue.begin();
        set<Value *>::iterator itSetEnd = setArrayValue.end();

        while (itSetBegin != itSetEnd) {
            (*itSetBegin)->dump();
            itSetBegin++;
        }
    } else if (isArrayAccessLoop1(pLoop, setArrayValue)) {
        errs() << "\nFOUND ARRAY ACCESSING LOOP\n";

        set<Value *>::iterator itSetBegin = setArrayValue.begin();
        set<Value *>::iterator itSetEnd = setArrayValue.end();

        while (itSetBegin != itSetEnd) {
            (*itSetBegin)->dump();
            itSetBegin++;
        }
    }


    set<Value *> setLinkedValue;

    if (isLinkedListAccessLoop(pLoop, setLinkedValue)) {
        errs() << "\nFOUND Linked List ACCESSING LOOP\n";
    }

    errs() << setLinkedValue.size() << "\n";

    set<Value *>::iterator itSetBegin = setLinkedValue.begin();
    set<Value *>::iterator itSetEnd = setLinkedValue.end();

    while (itSetBegin != itSetEnd) {
        (*itSetBegin)->dump();
        itSetBegin++;
    }

    return false;
}

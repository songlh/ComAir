#ifndef COMAIR_ARRAYLINKEDINDENTIFIER_H
#define COMAIR_ARRAYLINKEDINDENTIFIER_H

#include <set>

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Operator.h"

#include "Common/Helper.h"

using namespace llvm;
using namespace std;

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
    BasicBlock *fakePHeader = pHeader;

    if (setBlocks.find(pHeader) != setBlocks.end()) {
        return true;
    }

    if (!pHeader->getUniquePredecessor()) {
        // it may be built with  O2
        fakePHeader = pLoop->getLoopPredecessor();
    }

    set<BasicBlock *> setLoopBlocks;

    for (Loop::block_iterator BB = pLoop->block_begin(); BB != pLoop->block_end(); BB++) {
        BasicBlock *B = *BB;
        setLoopBlocks.insert(B);
    }

    vector<BasicBlock *> vecWorkList;
    set<BasicBlock *> setVisited;

    for (succ_iterator succ = succ_begin(pHeader); succ != succ_end(pHeader); succ++) {
        if (BranchInst *pBranch = dyn_cast<BranchInst>((*succ)->getTerminator())) {
            if (pBranch->isConditional()) {
                if (PHINode *pPHI = dyn_cast<PHINode>(pBranch->getCondition())) {
                    if (pPHI->getParent() == *succ) {
                        if (ConstantInt *pConstant = dyn_cast<ConstantInt>(
                                pPHI->getIncomingValue(pPHI->getBasicBlockIndex(pHeader)))) {
                            if (pConstant->isZero()) {
                                vecWorkList.push_back(pBranch->getSuccessor(1));
                                setVisited.insert(pBranch->getSuccessor(1));

                            } else {
                                vecWorkList.push_back(pBranch->getSuccessor(0));
                                setVisited.insert(pBranch->getSuccessor(0));
                            }

                            bool boolAllConstant = true;

                            for (unsigned i = 0; i < pPHI->getNumIncomingValues(); i++) {
                                if (!isa<ConstantInt>(pPHI->getIncomingValue(i))) {
                                    boolAllConstant = false;
                                    break;
                                }
                            }

                            if (boolAllConstant) {
                                setVisited.insert(*succ);
                            }

                            continue;
                        }
                    }
                }
            }
        }

        if (setLoopBlocks.find(*succ) != setLoopBlocks.end()) {
            vecWorkList.push_back(*succ);
            setVisited.insert(*succ);
        }
    }

    while (vecWorkList.size() > 0) {
        BasicBlock *pBlock = vecWorkList.back();
        vecWorkList.pop_back();

        if (pBlock == fakePHeader) {
            return false;
        }

        if (setBlocks.find(pBlock) != setBlocks.end()) {
            continue;
        }

        for (succ_iterator succ = succ_begin(pBlock); succ != succ_end(pBlock); succ++) {
            if (BranchInst *pBranch = dyn_cast<BranchInst>((*succ)->getTerminator())) {
                if (pBranch->isConditional()) {
                    if (PHINode *pPHI = dyn_cast<PHINode>(pBranch->getCondition())) {
                        if (pPHI->getParent() == *succ) {

                            if (ConstantInt *pConstant = dyn_cast<ConstantInt>(
                                    pPHI->getIncomingValue(pPHI->getBasicBlockIndex(pBlock)))) {
                                if (pConstant->isZero()) {
                                    vecWorkList.push_back(pBranch->getSuccessor(1));
                                    setVisited.insert(pBranch->getSuccessor(1));

                                } else {
                                    vecWorkList.push_back(pBranch->getSuccessor(0));
                                    setVisited.insert(pBranch->getSuccessor(0));
                                }

                                bool boolAllConstant = true;

                                for (unsigned i = 0; i < pPHI->getNumIncomingValues(); i++) {
                                    if (!isa<ConstantInt>(pPHI->getIncomingValue(i))) {
                                        boolAllConstant = false;
                                        break;
                                    }
                                }

                                if (boolAllConstant) {
                                    setVisited.insert(*succ);
                                }

                                continue;
                            }
                        }
                    }
                }

            }

            if (setLoopBlocks.find(*succ) != setLoopBlocks.end() && setVisited.find(*succ) == setVisited.end()) {
                vecWorkList.push_back(*succ);
                setVisited.insert(*succ);
            }
        }
    }

    return true;
}

bool isArrayAccessLoop1(Loop *pLoop, set<Value *> &setArrayValue) {
    map<Value *, vector<LoadInst *> > mapPArrValueAccess;
    set<BasicBlock *> setLoopBlocks;
    set<BasicBlock *> setLoopBlocksWithPre;

    for (Loop::block_iterator BB = pLoop->block_begin(); BB != pLoop->block_end(); BB++) {
        BasicBlock *B = *BB;
        setLoopBlocks.insert(B);
        setLoopBlocksWithPre.insert(B);
    }

    // for -O2
    BasicBlock *pHeader = pLoop->getHeader();
    if (!pHeader->getUniquePredecessor()) {
        BasicBlock *loopPre = pLoop->getLoopPredecessor();
        setLoopBlocksWithPre.insert(loopPre);
    }

    for (Loop::block_iterator BB = pLoop->block_begin(); BB != pLoop->block_end(); BB++) {
        BasicBlock *B = *BB;

        for (BasicBlock::iterator II = B->begin(); II != B->end(); II++) {
            if (LoadInst *pLoad = dyn_cast<LoadInst>(&*II)) {
                if (GetElementPtrInst *pSource = dyn_cast<GetElementPtrInst>(pLoad->getPointerOperand())) {
                    if (setLoopBlocksWithPre.find(pSource->getParent()) == setLoopBlocksWithPre.end()) {
                        continue;
                    }

                    Value *vArrayValue = pSource->getPointerOperand();
                    mapPArrValueAccess[vArrayValue].push_back(pLoad);

//                    vArrayValue->dump();
                }
            }
        }
    }

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
    errs() << setPArrValue.size() << "\n";

    set<Value *>::iterator itSetBegin = setPArrValue.begin();
    set<Value *>::iterator itSetEnd = setPArrValue.end();

    set<Value *> setElementPtrInst;

    while (itSetBegin != itSetEnd) {
        set<BasicBlock *> setStoreBlocks;
        setElementPtrInst.clear();

        for (User *U:  (*itSetBegin)->users()) {
            if (GetElementPtrInst *pGet = dyn_cast<GetElementPtrInst>(U)) {
                if (setLoopBlocksWithPre.find(pGet->getParent()) == setLoopBlocksWithPre.end()) {
                    continue;
                }

                if (pGet->getNumOperands() != 2) {
                    continue;
                }

                Value *vOffset = pGet->getOperand(1);

                while (CastInst *pCast = dyn_cast<CastInst>(vOffset)) {
                    vOffset = pCast->getOperand(0);
                }
//                vOffset->dump();

                if (LoadInst *pLoad = dyn_cast<LoadInst>(vOffset)) {
                    for (User *UOffset : pLoad->getOperand(0)->users()) {
                        if (StoreInst *pStore = dyn_cast<StoreInst>(UOffset)) {
                            if (setLoopBlocksWithPre.find(pStore->getParent()) != setLoopBlocksWithPre.end()) {
                                setStoreBlocks.insert(pStore->getParent());
                                setElementPtrInst.insert(pGet);
                            }
                        }
                    }
                } else if (OverflowingBinaryOperator *op = dyn_cast<OverflowingBinaryOperator>(vOffset)) {
                    Value *firstVal = op->getOperand(0);
                    firstVal->dump();
                    if (LoadInst *pLoad = dyn_cast<LoadInst>(firstVal)) {
                        for (User *UOffset : pLoad->getOperand(0)->users()) {
                            if (StoreInst *pStore = dyn_cast<StoreInst>(UOffset)) {
                                if (setLoopBlocksWithPre.find(pStore->getParent()) != setLoopBlocksWithPre.end()) {
                                    setStoreBlocks.insert(pStore->getParent());
                                    setElementPtrInst.insert(pGet);
                                }
                            }
                        }
                    } else if (PHINode *pPHI = dyn_cast<PHINode>(firstVal)) {
                        for (unsigned i = 0; i < pPHI->getNumIncomingValues(); i++) {
                            if (!isa<ConstantInt>(pPHI->getIncomingValue(i))) {
                                if (BinaryOperator *op2 = dyn_cast<BinaryOperator>(pPHI->getIncomingValue(i))) {
                                    Value *firstVal2 = op2->getOperand(0);

                                    if (firstVal2 == pPHI) {
                                        setElementPtrInst.insert(pGet);
                                    }
                                }
                            }
                        }
                    }

                } else if (PHINode *pPHI = dyn_cast<PHINode>(vOffset)) {
                    for (unsigned i = 0; i < pPHI->getNumIncomingValues(); i++) {
                        if (!isa<ConstantInt>(pPHI->getIncomingValue(i))) {
                            if (BinaryOperator *op = dyn_cast<BinaryOperator>(pPHI->getIncomingValue(i))) {
                                Value *firstVal = op->getOperand(0);

                                if (firstVal == pPHI) {
                                    setElementPtrInst.insert(pGet);
                                }
                            }
                        }
                    }
                }
            }
        }
//        errs() << setElementPtrInst.size() << "\n";
        if (isOneStarLoop(pLoop, setStoreBlocks)) {
            // FIXME::
            for (Value *elePtrInst: setElementPtrInst) {
                for (User *u : elePtrInst->users()) {
                    if (LoadInst *pLoad = dyn_cast<LoadInst>(u)) {
                        if (setLoopBlocks.find(pLoad->getParent()) == setLoopBlocks.end()) {
                            continue;
                        }
                        setArrayValue.insert(pLoad);
                    }
                }
            }
            //
//            setArrayValue.insert(*itSetBegin);
        }

        itSetBegin++;
    }
//    errs() << setArrayValue.size() << "\n";
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
                } else if (GetElementPtrInst *pGet = dyn_cast<GetElementPtrInst>(pLoad->getPointerOperand())) {
                    if (LoadInst *pSource = dyn_cast<LoadInst>(pGet->getPointerOperand())) {
                        if (setLoopBlocks.find(pSource->getParent()) == setLoopBlocks.end()) {
                            continue;
                        }

                        Value *vArrayValue = pSource->getPointerOperand();
                        mapPArrValueAccess[vArrayValue].push_back(pLoad);
                    }
                }
            }
        }
    }

//    errs() << mapPArrValueAccess.size() << "\n";

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

    set<Value *> setElementPtrInst;

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
                                setElementPtrInst.insert(pGetElem);
                            }
                        }
                    }
                }
            }
        }

        if (isOneStarLoop(pLoop, setStoreBlocks)) {
            // FIXME::
//            for (User *U:  (*itSetBegin)->users()) {
//                if (LoadInst *pLoad = dyn_cast<LoadInst>(U)) {
//                    if (setLoopBlocks.find(pLoad->getParent()) == setLoopBlocks.end()) {
//                        continue;
//                    }
//                    setArrayValue.insert(pLoad);
//                }
//            }

            for (Value *elePtrInst: setElementPtrInst) {
                for (User *u : elePtrInst->users()) {
                    if (LoadInst *pLoad = dyn_cast<LoadInst>(u)) {
                        if (setLoopBlocks.find(pLoad->getParent()) == setLoopBlocks.end()) {
                            continue;
                        }
                        setArrayValue.insert(pLoad);
                    } else if (StoreInst *pStore = dyn_cast<StoreInst>(u)) {
                        if (setLoopBlocks.find(pStore->getParent()) == setLoopBlocks.end()) {
                            continue;
                        }
                        setArrayValue.insert(pStore);
                    }
                }
            }

            //
//            setArrayValue.insert(*itSetBegin);
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

    // for -O2
    BasicBlock *loopPre = pLoop->getLoopPredecessor();
    setLoopBlocks.insert(loopPre);

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

    set<Value *> setElementPtrInst;

    set<Value *>::iterator itSetBegin = setPLLValue.begin();
    set<Value *>::iterator itSetEnd = setPLLValue.end();

    while (itSetBegin != itSetEnd) {
        set<BasicBlock *> setStoreBlocks;
        LoadInst *pLoadValue;
        setElementPtrInst.clear();

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
                                            pLoadValue = pLoad1;
                                            setStoreBlocks.insert(pStore->getParent());
                                            setElementPtrInst.insert(pGet);
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

                                        if (GetElementPtrInst *pGet = dyn_cast<GetElementPtrInst>(
                                                pLoad1->getPointerOperand())) {
                                            if (ConstantInt *CI = dyn_cast<ConstantInt>(pGet->getOperand(1))) {
                                                if (CI->isZero()) {
                                                    if (LoadInst *pLoad2 = dyn_cast<LoadInst>(
                                                            pGet->getPointerOperand())) {
                                                        if (pLoad2->getPointerOperand() == *itSetBegin) {
                                                            pLoadValue = pLoad2;
                                                            setStoreBlocks.insert(pStore->getParent());
                                                            setElementPtrInst.insert(pGet);
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

            // FIXME:: may be not right!!!
            for (Value *elePtrInst: setElementPtrInst) {
                for (User *u : elePtrInst->users()) {
                    if (LoadInst *pLoad = dyn_cast<LoadInst>(u)) {
                        if (setLoopBlocks.find(pLoad->getParent()) == setLoopBlocks.end()) {
                            continue;
                        }
                        setLinkedValue.insert(pLoad);
                    }
                }
            }

//            setLinkedValue.insert(pLoadValue);
        }

        itSetBegin++;
    }

    if (setLinkedValue.size() > 0) {
        return true;
    }

    return false;
}


void findArrayIndexAndData(Instruction *Inst) {
    // array 1
    if (LoadInst *loadInst = dyn_cast<LoadInst>(Inst)) {

        Value *Op1 = loadInst->getOperand(0);
        if (GetElementPtrInst *getPtr = dyn_cast<GetElementPtrInst>(Op1)) {

            Value *data = getPtr->getOperand(0);
            Value *index = getPtr->getOperand(1);

            Instruction *IndexInst = dyn_cast<Instruction>(index);

            // find index
            if (IndexInst->getOpcode() == Instruction::SExt) {

                SExtInst *sExtInst = dyn_cast<SExtInst>(index);
                Value *indexPre = sExtInst->getOperand(0);
                Instruction *indexInst = dyn_cast<Instruction>(indexPre);

                if (indexInst->getNumOperands() == 1) {

                    errs() << "Find index instruction :" << "\n";
                    indexPre->dump();

                } else {

                    Value *v1 = indexInst->getOperand(0);
                    Value *v2 = indexInst->getOperand(1);
                    if (ConstantInt *contInst = dyn_cast<ConstantInt>(v2)) {
                        errs() << "Find index instruction :" << "\n";
                        v1->dump();
                    }
                }

            } else {
                errs() << "index instruction is not first situation:" << "\n";
            }

            // find data
            if (LoadInst *dataLoad = dyn_cast<LoadInst>(data)) {

                errs() << "Find data instruction :" << "\n";
                dataLoad->dump();

            } else {
                errs() << "index instruction is not first situation:" << "\n";
            }

        }
    } else if (StoreInst *storeInst = dyn_cast<StoreInst>(Inst)) {
        // array 0
        Value *src = storeInst->getOperand(1);
        if (AllocaInst *allocSrc = dyn_cast<AllocaInst>(src)) {
            errs() << "Find src instruction :" << "\n";
            allocSrc->dump();
        }
    }
}

#endif //COMAIR_ARRAYLINKEDINDENTIFIER_H

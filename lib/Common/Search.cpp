#ifndef COMAIR_SEARCH_H
#define COMAIR_SEARCH_H

#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"

#include "Common/Search.h"

using namespace llvm;
using namespace std;


BasicBlock *SearchBlockByName(Function *pFunction, string sName) {
    for (Function::iterator BB = pFunction->begin(); BB != pFunction->end(); BB++) {
        if (BB->getName() == sName) {
            return &*BB;
        }
    }

    return NULL;
}

Function *SearchFunctionByName(Module &M, string strFunctionName) {

    for (Module::iterator FI = M.begin(); FI != M.end(); FI++) {

        Function *f = &*FI;

        if (f->getName().str() == strFunctionName) {
            return f;
        }

    }

    return NULL;
}


Function *SearchFunctionByName(Module &M, string &strFileName, string &strFunctionName, unsigned uSourceCodeLine) {

    Function *pFunction = NULL;
    char pPath[400];

    for (Module::iterator f = M.begin(), fe = M.end(); f != fe; f++) {
        //errs() << f->getName() << "\n";
        if (f->getName().find(strFunctionName) != std::string::npos) {
            //errs() << f->getName() << "\n";
            map<string, pair<unsigned int, unsigned int> > mapFunctionBound;

            for (Function::iterator b = f->begin(), be = f->end(); b != be; ++b) {

                for (BasicBlock::iterator i = b->begin(), ie = b->end(); i != ie; ++i) {

                    Instruction *pInst = &*i;
                    if (MDNode *N = i->getMetadata("dbg")) {

                        const DILocation *Loc = pInst->getDebugLoc();

                        string sFileNameForInstruction = Loc->getDirectory().str() + "/" + Loc->getFilename().str();
                        realpath(sFileNameForInstruction.c_str(), pPath);
                        sFileNameForInstruction = string(pPath);
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

            //if(mapFunctionBound.find(strFileName) != mapFunctionBound.end())
            //{
            //	if(mapFunctionBound[strFileName].first <= uSourceCodeLine && mapFunctionBound[strFileName].second >= uSourceCodeLine)
            //	{
            //		pFunction = f;
            //		break;
            //	}
            //}
            map<string, pair<unsigned int, unsigned int> >::iterator itMapBegin = mapFunctionBound.begin();
            map<string, pair<unsigned int, unsigned int> >::iterator itMapEnd = mapFunctionBound.end();
            while (itMapBegin != itMapEnd) {
                if (itMapBegin->first.find(strFileName) != string::npos) {
                    if (itMapBegin->second.first <= uSourceCodeLine && itMapBegin->second.second >= uSourceCodeLine) {
                        pFunction = &*f;
                        break;
                    }
                }
                itMapBegin++;
            }
        } // if( f->getName().find(strFunctionName) != std::string::npos )
    } // for( Module::iterator ...)

    return pFunction;
}


void SearchBasicBlocksByLineNo(Function *F, unsigned uLineNo, vector<BasicBlock *> &vecBasicBlocks) {
    for (Function::iterator b = F->begin(), be = F->end(); b != be; ++b) {
        BasicBlock *BB = &*b;
        for (BasicBlock::iterator i = b->begin(), ie = b->end(); i != ie; ++i) {
            Instruction *pInst = &*i;
            if (MDNode *N = i->getMetadata("dbg")) {
                const DILocation *Loc = pInst->getDebugLoc();
                //errs() << Loc.getLineNumber() << " " << b->getName() << "\n";
                if (uLineNo == Loc->getLine()) {
                    vecBasicBlocks.push_back(BB);
                    break;
                }
            }
        }
    }

}


Loop *SearchLoopByLineNo(Function *pFunction, LoopInfo *pLI, unsigned uLineNo) {
    vector<BasicBlock *> vecBasicBlocks;
    SearchBasicBlocksByLineNo(pFunction, uLineNo, vecBasicBlocks);
    unsigned int uDepth = 0;
    BasicBlock *pBlock = NULL;

    for (vector<BasicBlock *>::iterator itBegin = vecBasicBlocks.begin(), itEnd = vecBasicBlocks.end();
         itBegin != itEnd; itBegin++) {
        if (pLI->getLoopDepth(*itBegin) > uDepth) {
            uDepth = pLI->getLoopDepth(*itBegin);
            pBlock = *itBegin;
        }
    }

    if (pBlock == NULL) {
        //return NULL;
        set<Loop *> setLoop;
        for (Function::iterator BI = pFunction->begin(); BI != pFunction->end(); BI++) {
            BasicBlock *BB = &*BI;
            if (pLI->getLoopDepth(BB) > 0) {
                setLoop.insert(pLI->getLoopFor(BB));
            }
        }

        if (setLoop.size() == 1) {
            return *(setLoop.begin());
        }

        return NULL;
    }


    return pLI->getLoopFor(pBlock);

}


#endif //COMAIR_SEARCH_H

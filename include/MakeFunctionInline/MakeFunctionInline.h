//
// Created by tzt77 on 1/29/18.
//

#ifndef COMAIR_MAKEFUNCTIONINLINE_H
#define COMAIR_MAKEFUNCTIONINLINE_H

#include "llvm/Pass.h"


using namespace llvm;

struct MakeFunctionInline : public ModulePass
{
    static char ID;
    MakeFunctionInline();

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;
    virtual bool runOnModule(Module& M);

};


#endif //COMAIR_MAKEFUNCTIONINLINE_H

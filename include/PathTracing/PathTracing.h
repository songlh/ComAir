//===-------------------------- PathTracing.h -----------------------------===//
//
// This pass instruments functions to perform path tracing.  The method is
// based on Ball-Larus path profiling, originally published in [1].
//
// [1]
//  T. Ball and J. R. Larus. "Efficient Path Profiling."
//  International Symposium on Microarchitecture, pages 46-57, 1996.
//  http://portal.acm.org/citation.cfm?id=243857
//===----------------------------------------------------------------------===//
//
// Copyright (c) 2016 Peter J. Ohmann and Benjamin R. Liblit
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//===----------------------------------------------------------------------===//
//
//                      The LLVM Compiler Infrastructure
//
// This file is derived from the file PathProfiling.cpp, originally distributed
// with the LLVM Compiler Infrastructure, originally licensed under the
// University of Illinois Open Source License.  See UI_LICENSE.txt for details.
//
//===----------------------------------------------------------------------===//
#ifndef CSI_PATH_TRACING_H
#define CSI_PATH_TRACING_H

#include "PathTracing/PPBallLarusDag.h"

#include <llvm/Pass.h>

#include "llvm/IR/CFG.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Instructions.h"

#include <fstream>

using namespace llvm;


// ---------------------------------------------------------------------------
// PathTracing is a module pass which instruments based on Ball/Larus
// path profiling
// ---------------------------------------------------------------------------

struct PathTracing : public ModulePass {
private:
  // Current context for multi threading support.
  llvm::LLVMContext* Context;
  
  // Gets/sets the current path tracker and it's debug info
  llvm::Value* getPathTracker();
  void setPathTracker(llvm::Value* c);
  llvm::Value* _pathTracker; // The storage for the current path tracker
  
  std::ofstream trackerStream; // The output stream to the tracker file
                               // (managed by runOnFunction and written to as
                               // we go)

  // Analyzes and instruments the function for path tracing
  bool runOnFunction(llvm::Function &F);
  // Perform module-level tasks, open streams, and instrument each function
  bool runOnModule(llvm::Module &M);

  // Creates an increment constant representing incr.
  llvm::ConstantInt* createIncrementConstant(long incr, int bitsize);

  // Creates an increment constant representing the value in
  // edge->getIncrement().
  llvm::ConstantInt* createIncrementConstant(BLInstrumentationEdge* edge);

  // Creates a counter increment in the given node.  The Value* in node is
  // taken as the index into a hash table.
  void insertCounterIncrement(llvm::Value* incValue,
                              Instruction *insertPoint,
                              BLInstrumentationDag* dag);

  // Inserts instrumentation for the given edge
  //
  // Pre: The edge's source node has pathNumber set if edge is non zero
  // path number increment.
  //
  // Post: Edge's target node has a pathNumber set to the path number Value
  // corresponding to the value of the path register after edge's
  // execution.
  void insertInstrumentationStartingAt(
    BLInstrumentationEdge* edge,
    BLInstrumentationDag* dag);
  
  // If this edge is a critical edge, then inserts a node at this edge.
  // This edge becomes the first edge, and a new PPBallLarusEdge is created.
  bool splitCritical(BLInstrumentationEdge* edge, BLInstrumentationDag* dag);

  // Inserts instrumentation according to the marked edges in dag.  Phony
  // edges must be unlinked from the DAG, but accessible from the
  // backedges.  Dag must have initializations, path number increments, and
  // counter increments present.
  //
  // Counter storage is created here.
  void insertInstrumentation( BLInstrumentationDag& dag);
  
  // Writes the tracker increments (and initial tracker values)
  void writeTrackerInfo(llvm::Function& F, BLInstrumentationDag* dag);
  
  // Writes out bb #s mapped to their source line numbers
  void writeBBs(llvm::Function& F, BLInstrumentationDag* dag);

public:
  static char ID; // Pass identification, replacement for typeid
  PathTracing() : ModulePass(ID) {}

  void getAnalysisUsage(llvm::AnalysisUsage &) const;
};

#endif

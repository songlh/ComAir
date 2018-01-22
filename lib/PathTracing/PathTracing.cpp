#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"


#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#include <climits>
#include <iostream>
#include <list>
#include <set>

#include "PathTracing/BLInstrumentationGag.h"
#include "PathTracing/PathTracing.h"

using namespace llvm;
using namespace std;

unsigned long HASH_THRESHHOLD;
unsigned int PATHS_SIZE;

// a special argument parser for unsigned longs
class ULongParser : public cl::parser<unsigned long> {
public:
    // parse - Return true on error.
    bool parse(cl::Option &O, const StringRef ArgName, const StringRef &Arg,
               unsigned long &Val){
        (void)ArgName;
        if(Arg.getAsInteger(0, Val))
            return O.error("'" + Arg + "' value invalid for ulong argument!");
        return false;
    }

    // getValueName - Say what type we are expecting
    const char* getValueName() const{
        return "ulong";
    }
};

static cl::opt<bool> SilentInternal("pt-silent", cl::desc("Silence internal "
                                                                  "warnings.  Will still print errors which "
                                                                  "cause PT to fail."));

static cl::opt<bool> NoArrayWrites("pt-no-array-writes", cl::Hidden,
                                   cl::desc("Don't instrument loads and stores to the path array. "
                                                    "DEBUG ONLY"));

static cl::opt<unsigned long, false, ULongParser> HashSize("pt-hash-size",
                                                           cl::desc("Set the maximum acyclic path count "
                                                                            "to instrument per function. "
                                                                            "Default: ULONG_MAX / 2 - 1"),
                                                           cl::value_desc("hash_size"));

static cl::opt<int> ArraySize("pt-path-array-size", cl::desc("Set the size "
                                                                     "of the paths array for instrumented "
                                                                     "functions.  Default: 10"),
                              cl::value_desc("path_array_size"));

static cl::opt<string> TrackerFile("pt-info-file", cl::desc("The path to "
                                                                    "the increment-line-number output file."),
                                   cl::value_desc("file_path"));


// Register path tracing as a pass
char PathTracing::ID = 0;

static RegisterPass<PathTracing> X(
        "pt-inst",
        "Insert instrumentation for Ball-Larus tracing",
        false, false);

// Creates an increment constant representing incr.
ConstantInt *PathTracing::createIncrementConstant(long incr,
                                                  int bitsize) {
    return (ConstantInt::get(IntegerType::get(*Context, bitsize), incr));
}

// Creates an increment constant representing the value in
// edge->getIncrement().
ConstantInt *PathTracing::createIncrementConstant(
        BLInstrumentationEdge *edge) {
    return (createIncrementConstant(edge->getIncrement(), 64));
}

// Creates a counter increment in the given node.  The Value* in node is
// taken as the index into an array or hash table.  The hash table access
// is a call to the runtime.
void PathTracing::insertCounterIncrement(Value *incValue,
                                         Instruction *insertPoint,
                                         BLInstrumentationDag *dag) {
    // first, find out the current location
    LoadInst *curLoc = new LoadInst(dag->getCurIndex(), "curIdx", false, insertPoint);

    // Counter increment for array
    if (dag->getNumberOfPaths() <= HASH_THRESHHOLD) {
        // Get pointer to the array location
        Value *const gepIndices[] = {
                Constant::getNullValue(Type::getInt64Ty(*Context)),
                curLoc,
        };

        GetElementPtrInst *pcPointer =
                GetElementPtrInst::CreateInBounds(dag->getCounterArray(), gepIndices,
                                                  "arrLoc", insertPoint);

        // Store back in to the array
        new StoreInst(incValue, pcPointer, true, insertPoint);

        // update the next circular buffer location
        Type *tInt = Type::getInt64Ty(*Context);
        BinaryOperator *addLoc = BinaryOperator::Create(Instruction::Add,
                                                        curLoc,
                                                        ConstantInt::get(tInt, 1),
                                                        "addLoc", insertPoint);
        Instruction *atEnd = new ICmpInst(insertPoint, CmpInst::ICMP_EQ, curLoc,
                                          ConstantInt::get(tInt, dag->getCounterSize() - 1),
                                          "atEnd");
        Instruction *nextLoc = SelectInst::Create(atEnd, ConstantInt::get(tInt, 0),
                                                  addLoc, "nextLoc", insertPoint);

        new StoreInst(nextLoc, dag->getCurIndex(), true, insertPoint);
        new StoreInst(ConstantInt::get(tInt, 0), this->getPathTracker(), true,
                      insertPoint);
    } else {
        // Counter increment for hash would have gone here (should actually be
        // unreachable now)
        errs() << "ERROR: instrumentation continued in function with over-large"
               << " path count.  This is a tool error. Results will be wrong!\n";
        exit(1);
    }
}

void PathTracing::insertInstrumentationStartingAt(BLInstrumentationEdge *edge,
                                                  BLInstrumentationDag *dag) {
    // Mark the edge as instrumented
    edge->setHasInstrumentation(true);

    // create a new node for this edge's instrumentation
    splitCritical(edge, dag);

    BLInstrumentationNode *sourceNode = (BLInstrumentationNode *) edge->getSource();
    BLInstrumentationNode *targetNode = (BLInstrumentationNode *) edge->getTarget();
    BLInstrumentationNode *instrumentNode;

    bool atBeginning = false;

    // Source node has only 1 successor so any information can be simply
    // inserted in to it without splitting
    if (sourceNode->getBlock() && sourceNode->getNumberSuccEdges() <= 1) {
        errs() << "  Potential instructions to be placed in: "
               << sourceNode->getName() << " (at end)\n";
        instrumentNode = sourceNode;
    }

        // The target node only has one predecessor, so we can safely insert edge
        // instrumentation into it. If there was splitting, it must have been
        // successful.
    else if (targetNode->getNumberPredEdges() == 1) {
        errs() << "  Potential instructions to be placed in: "
               << targetNode->getName() << " (at beginning)\n";
        instrumentNode = targetNode;
        atBeginning = true;
    }

        // Somehow, splitting must have failed.
    else {
        errs() << "ERROR: Instrumenting could not split a critical edge.\n";
        exit(1);
    }

    // Insert instrumentation if this is a back or split edge
    if (edge->getType() == PPBallLarusEdge::BACKEDGE ||
        edge->getType() == PPBallLarusEdge::SPLITEDGE) {
        BLInstrumentationEdge *top =
                (BLInstrumentationEdge *) edge->getPhonyRoot();
        BLInstrumentationEdge *bottom =
                (BLInstrumentationEdge *) edge->getPhonyExit();

        assert(top->isInitialization() && " Top phony edge did not"
                " contain a path number initialization.");
        assert(bottom->isCounterIncrement() && " Bottom phony edge"
                " did not contain a path counter increment.");

        Instruction *Inst;

        if (atBeginning) {
            Inst = &*instrumentNode->getBlock()->getFirstInsertionPt();
        } else {
            Inst = instrumentNode->getBlock()->getTerminator();
        }

        // add information from the bottom edge, if it exists
        if (bottom->getIncrement()) {
            Instruction *oldValue = new LoadInst(this->getPathTracker(),
                                                 "oldValBackSplit", Inst);
            Instruction *newValue = BinaryOperator::Create(Instruction::Add,
                                                           oldValue, createIncrementConstant(bottom),
                                                           "pathNumber", Inst);
            new StoreInst(newValue, this->getPathTracker(), true, Inst);
        }

        if (!NoArrayWrites) {
            Instruction *curValue = new LoadInst(this->getPathTracker(),
                                                 "curValBackSplit", Inst);
            insertCounterIncrement(curValue, Inst, dag);
        }

        new StoreInst(createIncrementConstant(top), this->getPathTracker(), true,
                      Inst);

        // Check for path counter increments: we would never expect the top edge to
        // also be a counter increment, but we'll handle it anyways
        if (top->isCounterIncrement()) {
            errs() << "WARNING: a top counter increment encountered\n";
            if (!NoArrayWrites) {
                insertCounterIncrement(createIncrementConstant(top),
                                       instrumentNode->getBlock()->getTerminator(), dag);
            }
        }
    }

        // Insert instrumentation if this is a normal edge
    else {

        Instruction *Inst;

        if (atBeginning) {
            Inst = &*instrumentNode->getBlock()->getFirstInsertionPt();
        } else {
            Inst = instrumentNode->getBlock()->getTerminator();
        }

        if (edge->isInitialization()) { // initialize path number
            new StoreInst(createIncrementConstant(edge),
                          this->getPathTracker(), true,
                          Inst);
        } else if (edge->getIncrement()) {// increment path number
            Instruction *oldValue = new LoadInst(this->getPathTracker(),
                                                 "oldVal", Inst);
            Instruction *newValue = BinaryOperator::Create(Instruction::Add,
                                                           oldValue, createIncrementConstant(edge),
                                                           "pathNumber", Inst);
            new StoreInst(newValue, this->getPathTracker(), true, Inst);
        }

        // Check for path counter increments (function exit)
        if (edge->isCounterIncrement()) {
            if (!NoArrayWrites) {
                Instruction *curValue = new LoadInst(this->getPathTracker(),
                                                     "curVal", Inst);
                insertCounterIncrement(curValue, Inst, dag);
            }
        }
    }

    // Add all the successors
    for (PPBLEdgeIterator next = targetNode->succBegin(),
                 end = targetNode->succEnd(); next != end; next++) {
        // So long as it is un-instrumented, add it to the list
        if (!((BLInstrumentationEdge *) (*next))->hasInstrumentation())
            insertInstrumentationStartingAt((BLInstrumentationEdge *) *next, dag);
    }
}

void PathTracing::insertInstrumentation(BLInstrumentationDag &dag) {

    BLInstrumentationEdge *exitRootEdge =
            (BLInstrumentationEdge *) dag.getExitRootEdge();
    insertInstrumentationStartingAt(exitRootEdge, &dag);

    PPBLEdgeVector callEdges = dag.getCallPhonyEdges();
    if (callEdges.begin() != callEdges.end()) {
        errs() << "ERROR: phony edges were inserted for calls.  This is not "
               << "supported for path tracing.  Do not use flag "
               << "'-path-profile-early-termination'\n";
        exit(1);
    }
}

// If this edge is a critical edge, then inserts a node at this edge.
// This edge becomes the first edge, and a new PPBallLarusEdge is created.
// Returns true if the edge was split
bool PathTracing::splitCritical(BLInstrumentationEdge *edge,
                                BLInstrumentationDag *dag) {
    unsigned succNum = edge->getSuccessorNumber();
    PPBallLarusNode *sourceNode = edge->getSource();
    PPBallLarusNode *targetNode = edge->getTarget();
    BasicBlock *sourceBlock = sourceNode->getBlock();
    BasicBlock *targetBlock = targetNode->getBlock();

    if (sourceBlock == NULL || targetBlock == NULL
        || sourceNode->getNumberSuccEdges() <= 1
        || targetNode->getNumberPredEdges() == 1) {
        return (false);
    }

    TerminatorInst *terminator = sourceBlock->getTerminator();

    if (SplitCriticalEdge(terminator, succNum)) {
        BasicBlock *newBlock = terminator->getSuccessor(succNum);
        dag->splitUpdate(edge, newBlock);
        return (true);
    } else
        return (false);
}

// NOTE: could handle inlining specially here if desired.
static void writeBBLineNums(BasicBlock *bb,
                            BLInstrumentationDag *dag,
                            raw_ostream &stream = outs()) {
    if (!bb) {
        stream << "|NULL";
        return;
    }

    bool any = false;
    DebugLoc dbLoc;
    for (BasicBlock::iterator i = bb->begin(), e = bb->end(); i != e; ++i) {
        if (BranchInst *inst = dyn_cast<BranchInst>(i))
            if (inst->isUnconditional())
                continue;

        dbLoc = i->getDebugLoc();
        if (!dbLoc.isUnknown()) {
            stream << '|' << dbLoc.getLine(); // << ':' << dbLoc.getCol();
            any = true;
        } else if (LoadInst *inst = dyn_cast<LoadInst>(&*i)) {
            if (inst->getPointerOperand() == dag->getCurIndex() &&
                inst->getName().find("curIdx") == 0) {
                stream << "|-1";
                any = true;
            }
        }
    }

    // write NULL if there are no debug locations in the basic block
    if (!any)
        stream << "|NULL";
}


static void writeBBLineNums(BasicBlock *bb, BLInstrumentationDag *dag, ostream &stream) {
    raw_os_ostream wrapped(stream);
    writeBBLineNums(bb, dag, wrapped);
}


void PathTracing::writeBBs(Function &F, BLInstrumentationDag *dag) {
    BLInstrumentationEdge *root = (BLInstrumentationEdge *) dag->getExitRootEdge();
    list<BLInstrumentationEdge *> edgeWl;
    set<BLInstrumentationNode *> done;
    //edgeWl.push_back(root);

    // special handling here for exit node (mark as "EXIT")
    BLInstrumentationNode *exitNode = (BLInstrumentationNode *) root->getSource();
    trackerStream << exitNode->getNodeId() << "|EXIT" << '\n';
    if (exitNode->getBlock()) {
        errs() << "ERROR: Exit node has associated basic block in function "
               << F.getName() << ".  This is a tool error.\n";
        exit(1);
    }
    done.insert(exitNode);

    // special handling here for entry node (mark as "ENTRY" but also write line#)
    BLInstrumentationNode *entryNode = (BLInstrumentationNode *) root->getTarget();
    trackerStream << entryNode->getNodeId() << "|ENTRY";
    writeBBLineNums(entryNode->getBlock(), dag, trackerStream);
    trackerStream << '\n';
    for (PPBLEdgeIterator i = entryNode->succBegin(), e = entryNode->succEnd(); i != e; ++i) {
        edgeWl.push_back((BLInstrumentationEdge *) (*i));
    }
    done.insert(entryNode);

    while (!edgeWl.empty()) {
        BLInstrumentationNode *current =
                (BLInstrumentationNode *) edgeWl.front()->getTarget();
        edgeWl.pop_front();
        if (done.count(current))
            continue;

        trackerStream << current->getNodeId();
        writeBBLineNums(current->getBlock(), dag, trackerStream);
        trackerStream << '\n';

        for (PPBLEdgeIterator i = current->succBegin(), e = current->succEnd(); i != e; ++i) {
            edgeWl.push_back((BLInstrumentationEdge *) (*i));
        }

        done.insert(current);
    }
}

void PathTracing::writeTrackerInfo(Function &F, BLInstrumentationDag *dag) {
    trackerStream << "#\n" << F.getName().str() << '\n';

    writeBBs(F, dag);
    trackerStream << "$\n";

    BLInstrumentationEdge *root = (BLInstrumentationEdge *) dag->getExitRootEdge();
    list<BLInstrumentationEdge *> edgeWl;
    set<BLInstrumentationEdge *> done;

    // handle the root node specially
    BLInstrumentationNode *rTarget = (BLInstrumentationNode *) root->getTarget();
    for (PPBLEdgeIterator i = rTarget->succBegin(), e = rTarget->succEnd(); i != e; ++i) {
        edgeWl.push_back((BLInstrumentationEdge *) (*i));
    }
    done.insert(root);

    while (!edgeWl.empty()) {
        BLInstrumentationEdge *current = edgeWl.front();
        edgeWl.pop_front();
        if (done.count(current))
            continue;

        BLInstrumentationNode *source = (BLInstrumentationNode *) current->getSource();
        BLInstrumentationNode *target = (BLInstrumentationNode *) current->getTarget();

        if (current->getType() == PPBallLarusEdge::BACKEDGE ||
            current->getType() == PPBallLarusEdge::SPLITEDGE) {
            BLInstrumentationEdge *phony =
                    (BLInstrumentationEdge *) current->getPhonyRoot();
            trackerStream << source->getNodeId() << "~>" << target->getNodeId()
                          << '|' << phony->getIncrement()
                          << '$' << phony->getWeight() << '\n';
        } else {
            trackerStream << source->getNodeId() << "->" << target->getNodeId()
                          << '|' << current->getIncrement()
                          << '$' << current->getWeight()
                          << '\n';
        }

        for (PPBLEdgeIterator i = target->succBegin(), e = target->succEnd(); i != e; ++i) {
            edgeWl.push_back((BLInstrumentationEdge *) (*i));
        }

        done.insert(current);
    }
}


// Entry point of the function
bool PathTracing::runOnFunction(Function &F) {
    PrepareCSI &instData = getAnalysis<PrepareCSI>();
    if (F.isDeclaration() || !instData.hasInstrumentationType(F, "PT"))
        return false;
    errs() << "Function: " << F.getName() << '\n';

    // Build DAG from CFG
    BLInstrumentationDag dag = BLInstrumentationDag(F);
    dag.init();

    // give each path a unique integer value
    dag.calculatePathNumbers();

    // modify path increments to increase the efficiency
    // of instrumentation
    dag.calculateSpanningTree();
    dag.calculateChordIncrements();
    dag.pushInitialization();
    dag.pushCounters();
    dag.unlinkPhony();

    // We only support the information in an array for now
    if (dag.getNumberOfPaths() <= HASH_THRESHHOLD) {
        if (dag.error_negativeIncrements()) {
            errs() << "ERROR: Instrumentation is proceeding while DAG structure is "
                   << "in error and contains a negative increment for function "
                   << F.getName() << ".  This is a tool error.\n";
            exit(1);
        } else if (dag.error_edgeOverflow()) {
            errs() << "ERROR: Instrumentation is proceeding while DAG structure is "
                   << "in error due to an edge weight overflow for function "
                   << F.getName() << ".  This is a tool error.\n";
            exit(2);
        }

        Type *tInt = Type::getInt64Ty(*Context);
        Type *tArr = ArrayType::get(tInt, PATHS_SIZE);

        // declare the path index and array
        Instruction *entryInst = F.getEntryBlock().getFirstNonPHI();
        AllocaInst *arrInst = new AllocaInst(tArr, 0, "__PT_pathArr", entryInst);
        AllocaInst *idxInst = new AllocaInst(tInt, 0, "__PT_arrIndex", entryInst);
        new StoreInst(ConstantInt::get(tInt, 0), idxInst, true, entryInst);
        Instruction *trackInst = new AllocaInst(tInt, 0, "__PT_curPath", entryInst);
        new StoreInst(ConstantInt::get(tInt, 0), trackInst, true, entryInst);

        // Store the setinal value (-1) into pathArr[end]
        Value *const gepIndices[] = {
                Constant::getNullValue(Type::getInt64Ty(*Context)),
                ConstantInt::get(tInt, PATHS_SIZE - 1),
        };
        GetElementPtrInst *arrLast = GetElementPtrInst::CreateInBounds(arrInst, gepIndices,
                                                                       "arrLast", entryInst);
        new StoreInst(ConstantInt::get(tInt, -1), arrLast, true, entryInst);

        dag.setCounterArray(arrInst);
        dag.setCurIndex(idxInst);
        dag.setCounterSize(PATHS_SIZE);
        this->setPathTracker(trackInst);

        // create debug info for new variables
        DIBuilder Builder(*F.getParent());
        DIType intType = Builder.createBasicType("__pt_int", 64, 64, dwarf::DW_ATE_signed);
        const DIType arrType = createArrayType(Builder, PATHS_SIZE, intType);

        // get the debug location of any instruction in this basic block--this will
        // use the same info.  If there is none, technically we should build it, but
        // that's a huge pain (if it's possible) so I just give up right now
        const DebugLoc dbLoc = findEarlyDebugLoc(F, SilentInternal);
        if (!dbLoc.isUnknown()) {
            DIVariable arrDI = Builder.createLocalVariable(
                    (unsigned) dwarf::DW_TAG_auto_variable,
                    DIDescriptor(dbLoc.getScope(*Context)),
                    "__PT_counter_arr",
                    DIFile(dbLoc.getScope(*Context)), 0, arrType, true);
            insertDeclare(Builder, arrInst, arrDI, dbLoc, entryInst);
            DIVariable idxDI = Builder.createLocalVariable(
                    (unsigned) dwarf::DW_TAG_auto_variable,
                    DIDescriptor(dbLoc.getScope(*Context)),
                    "__PT_counter_idx",
                    DIFile(dbLoc.getScope(*Context)), 0, intType, true);
            insertDeclare(Builder, idxInst, idxDI, dbLoc, entryInst);
            DIVariable trackDI = Builder.createLocalVariable(
                    (unsigned) dwarf::DW_TAG_auto_variable,
                    DIDescriptor(dbLoc.getScope(*Context)),
                    "__PT_current_path",
                    DIFile(dbLoc.getScope(*Context)), 0, intType,
                    true);
            insertDeclare(Builder, trackInst, trackDI, dbLoc, entryInst);
        }

        // do the instrumentation and write out the path info to the .info file
        insertInstrumentation(dag);

        writeTrackerInfo(F, &dag);
    } else if (!SilentInternal) {
        errs() << "WARNING: instrumentation not done for function "
               << F.getName() << " due to large path count.  Path info "
               << "will be missing!\n";
        return false;
    }

    return true;
}

bool PathTracing::runOnModule(Module &M) {
    static bool runBefore = false;
    if (runBefore)
        return (false);
    runBefore = true;

    if (TrackerFile.empty())
        report_fatal_error("PT cannot continue: -pt-tracker-file [file] is required", false);
    trackerStream.open(TrackerFile.c_str(), ios::out | ios::trunc);
    if (!trackerStream.is_open())
        report_fatal_error("unable to open pt-file location: " + TrackerFile);
    errs() << "Output stream opened to " << TrackerFile << '\n';

    this->Context = &M.getContext();

    if (ArraySize > 0)
        PATHS_SIZE = ArraySize;
    else
        PATHS_SIZE = 10;

    if (HashSize > 0)
        HASH_THRESHHOLD = HashSize;
    else
        HASH_THRESHHOLD = ULONG_MAX / 2 - 1;

    bool changed = false;
    for (Module::iterator i = M.begin(), e = M.end(); i != e; ++i) {
        changed |= runOnFunction(*i);
    }

    trackerStream.close();
    return changed;
}
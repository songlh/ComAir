#define DEBUG_TYPE "path-tracing"

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

using namespace llvm;
using namespace std;

// Creates a new BLInstrumentationNode from a BasicBlock.
BLInstrumentationNode::BLInstrumentationNode(BasicBlock* BB) :
        PPBallLarusNode(BB),
        _startingPathNumber(NULL), _endingPathNumber(NULL), _pathPHI(NULL){
    static unsigned nextID = 0;
    _blockId = nextID++;
}

// Constructor for BLInstrumentationEdge.
BLInstrumentationEdge::BLInstrumentationEdge(BLInstrumentationNode* source,
                                             BLInstrumentationNode* target)
        : PPBallLarusEdge(source, target, 0),
          _increment(0), _isInSpanningTree(false), _isInitialization(false),
          _isCounterIncrement(false), _hasInstrumentation(false) {}

// Sets the target node of this edge.  Required to split edges.
void BLInstrumentationEdge::setTarget(PPBallLarusNode* node) {
    _target = node;
}

// Returns whether this edge is in the spanning tree.
bool BLInstrumentationEdge::isInSpanningTree() const {
    return(_isInSpanningTree);
}

// Sets whether this edge is in the spanning tree.
void BLInstrumentationEdge::setIsInSpanningTree(bool isInSpanningTree) {
    _isInSpanningTree = isInSpanningTree;
}

// Returns whether this edge will be instrumented with a path number
// initialization.
bool BLInstrumentationEdge::isInitialization() const {
    return(_isInitialization);
}

// Sets whether this edge will be instrumented with a path number
// initialization.
void BLInstrumentationEdge::setIsInitialization(bool isInitialization) {
    _isInitialization = isInitialization;
}

// Returns whether this edge will be instrumented with a path counter
// increment.  Notice this is incrementing the path counter
// corresponding to the path number register.  The path number
// increment is determined by getIncrement().
bool BLInstrumentationEdge::isCounterIncrement() const {
    return(_isCounterIncrement);
}

// Sets whether this edge will be instrumented with a path counter
// increment.
void BLInstrumentationEdge::setIsCounterIncrement(bool isCounterIncrement) {
    _isCounterIncrement = isCounterIncrement;
}

// Gets the path number increment that this edge will be instrumented
// with.  This is distinct from the path counter increment and the
// weight.  The counter increment is counts the number of executions of
// some path, whereas the path number keeps track of which path number
// the program is on.
long BLInstrumentationEdge::getIncrement() const {
    return(_increment);
}

// Set whether this edge will be instrumented with a path number
// increment.
void BLInstrumentationEdge::setIncrement(long increment) {
    if(increment < 0){
        DEBUG(dbgs() << "WARNING: we are setting a negative increment.  This is "
                     << "abnormal for an instrumented function.\n");
    }
    _increment = increment;
}

// True iff the edge has already been instrumented.
bool BLInstrumentationEdge::hasInstrumentation() {
    return(_hasInstrumentation);
}

// Set whether this edge has been instrumented.
void BLInstrumentationEdge::setHasInstrumentation(bool hasInstrumentation) {
    _hasInstrumentation = hasInstrumentation;
}

// Returns the successor number of this edge in the source.
unsigned BLInstrumentationEdge::getSuccessorNumber() {
    PPBallLarusNode* sourceNode = getSource();
    PPBallLarusNode* targetNode = getTarget();
    BasicBlock* source = sourceNode->getBlock();
    BasicBlock* target = targetNode->getBlock();

    if(source == NULL || target == NULL)
        return(0);

    TerminatorInst* terminator = source->getTerminator();

    unsigned i;
    for(i=0; i < terminator->getNumSuccessors(); i++) {
        if(terminator->getSuccessor(i) == target)
            break;
    }

    return(i);
}

// BLInstrumentationDag constructor initializes a DAG for the given Function.
BLInstrumentationDag::BLInstrumentationDag(Function &F) : PPBallLarusDag(F),
                                                          _curIndex(0),
                                                          _counterSize(0),
                                                          _counterArray(0),
                                                          _errorNegativeIncrements(false) {
}

// Returns the Exit->Root edge. This edge is required for creating
// directed cycles in the algorithm for moving instrumentation off of
// the spanning tree
PPBallLarusEdge* BLInstrumentationDag::getExitRootEdge() {
    PPBLEdgeIterator erEdge = getExit()->succBegin();
    return(*erEdge);
}

PPBLEdgeVector BLInstrumentationDag::getCallPhonyEdges () {
    PPBLEdgeVector callEdges;

    for( PPBLEdgeIterator edge = _edges.begin(), end = _edges.end();
         edge != end; edge++ ) {
        if( (*edge)->getType() == PPBallLarusEdge::CALLEDGE_PHONY )
            callEdges.push_back(*edge);
    }

    return callEdges;
}

// Gets the path counter array
Value* BLInstrumentationDag::getCounterArray() {
    return _counterArray;
}

// Get the current counter index
Value* BLInstrumentationDag::getCurIndex() {
    return _curIndex;
}

// Get the circular array size
int BLInstrumentationDag::getCounterSize() {
    return _counterSize;
}

// Set the circular array
void BLInstrumentationDag::setCounterArray(Value* c) {
    _counterArray = c;
}

// Set the current counter index
void BLInstrumentationDag::setCurIndex(Value* c) {
    _curIndex = c;
}

// Set the circular array size
void BLInstrumentationDag::setCounterSize(int s) {
    _counterSize = s;
}


// Calculates the increment for the chords, thereby removing
// instrumentation from the spanning tree edges. Implementation is based on
// the algorithm in Figure 4 of [Ball94]
void BLInstrumentationDag::calculateChordIncrements() {
    calculateChordIncrementsDfs(0, getRoot(), NULL);

    BLInstrumentationEdge* chord;
    for(PPBLEdgeIterator chordEdge = _chordEdges.begin(),
                end = _chordEdges.end(); chordEdge != end; chordEdge++) {
        chord = (BLInstrumentationEdge*) *chordEdge;
        long increment = chord->getIncrement() + chord->getWeight();
        if(increment < 0)
            this->_errorNegativeIncrements = true;
        chord->setIncrement(increment);
    }
}

// Updates the state when an edge has been split
void BLInstrumentationDag::splitUpdate(BLInstrumentationEdge* formerEdge,
                                       BasicBlock* newBlock) {
    PPBallLarusNode* oldTarget = formerEdge->getTarget();
    PPBallLarusNode* newNode = addNode(newBlock);
    formerEdge->setTarget(newNode);
    newNode->addPredEdge(formerEdge);

    oldTarget->removePredEdge(formerEdge);
    PPBallLarusEdge* newEdge = addEdge(newNode, oldTarget,0);

    if( formerEdge->getType() == PPBallLarusEdge::BACKEDGE ||
        formerEdge->getType() == PPBallLarusEdge::SPLITEDGE) {
        newEdge->setType(formerEdge->getType());
        newEdge->setPhonyRoot(formerEdge->getPhonyRoot());
        newEdge->setPhonyExit(formerEdge->getPhonyExit());
        formerEdge->setType(PPBallLarusEdge::NORMAL);
        formerEdge->setPhonyRoot(NULL);
        formerEdge->setPhonyExit(NULL);
    }
}

// Calculates a spanning tree of the DAG ignoring cycles.  Whichever
// edges are in the spanning tree will not be instrumented, but this
// implementation does not try to minimize the instrumentation overhead
// by trying to find hot edges.
void BLInstrumentationDag::calculateSpanningTree() {
    stack<PPBallLarusNode*> dfsStack;

    for(PPBLNodeIterator nodeIt = _nodes.begin(), end = _nodes.end();
        nodeIt != end; nodeIt++) {
        (*nodeIt)->setColor(PPBallLarusNode::WHITE);
    }

    dfsStack.push(getRoot());
    while(!dfsStack.empty()) {
        PPBallLarusNode* node = dfsStack.top();
        dfsStack.pop();

        if(node->getColor() == PPBallLarusNode::WHITE)
            continue;

        PPBallLarusNode* nextNode;
        bool forward = true;
        PPBLEdgeIterator succEnd = node->succEnd();

        node->setColor(PPBallLarusNode::WHITE);
        // first iterate over successors then predecessors
        for(PPBLEdgeIterator edge = node->succBegin(), predEnd = node->predEnd();
            edge != predEnd; edge++) {
            if(edge == succEnd) {
                edge = node->predBegin();
                forward = false;
            }

            // Ignore split edges
            if ((*edge)->getType() == PPBallLarusEdge::SPLITEDGE)
                continue;

            nextNode = forward? (*edge)->getTarget(): (*edge)->getSource();
            if(nextNode->getColor() != PPBallLarusNode::WHITE) {
                nextNode->setColor(PPBallLarusNode::WHITE);
                makeEdgeSpanning((BLInstrumentationEdge*)(*edge));
            }
        }
    }

    for(PPBLEdgeIterator edge = _edges.begin(), end = _edges.end();
        edge != end; edge++) {
        BLInstrumentationEdge* instEdge = (BLInstrumentationEdge*) (*edge);
        // safe since createEdge is overriden
        if(!instEdge->isInSpanningTree() && (*edge)->getType()
                                            != PPBallLarusEdge::SPLITEDGE)
            _chordEdges.push_back(instEdge);
    }
}

// Pushes initialization further down in order to group the first
// increment and initialization.
void BLInstrumentationDag::pushInitialization() {
    BLInstrumentationEdge* exitRootEdge =
            (BLInstrumentationEdge*) getExitRootEdge();
    exitRootEdge->setIsInitialization(true);
    pushInitializationFromEdge(exitRootEdge);
}

// Pushes the path counter increments up in order to group the last path
// number increment.
void BLInstrumentationDag::pushCounters() {
    BLInstrumentationEdge* exitRootEdge =
            (BLInstrumentationEdge*) getExitRootEdge();
    exitRootEdge->setIsCounterIncrement(true);
    pushCountersFromEdge(exitRootEdge);
}

// Removes phony edges from the successor list of the source, and the
// predecessor list of the target.
void BLInstrumentationDag::unlinkPhony() {
    PPBallLarusEdge* edge;

    for(PPBLEdgeIterator next = _edges.begin(),
                end = _edges.end(); next != end; next++) {
        edge = (*next);

        if( edge->getType() == PPBallLarusEdge::BACKEDGE_PHONY ||
            edge->getType() == PPBallLarusEdge::SPLITEDGE_PHONY ||
            edge->getType() == PPBallLarusEdge::CALLEDGE_PHONY ) {
            unlinkEdge(edge);
        }
    }
}

// Allows subclasses to determine which type of Node is created.
// Override this method to produce subclasses of PPBallLarusNode if
// necessary. The destructor of PPBallLarusDag will call free on each pointer
// created.
PPBallLarusNode* BLInstrumentationDag::createNode(BasicBlock* BB) {
    return( new BLInstrumentationNode(BB) );
}

// Allows subclasses to determine which type of Edge is created.
// Override this method to produce subclasses of PPBallLarusEdge if
// necessary. The destructor of PPBallLarusDag will call free on each pointer
// created.
PPBallLarusEdge* BLInstrumentationDag::createEdge(PPBallLarusNode* source,
                                                  PPBallLarusNode* target, unsigned edgeNumber) {
    (void)edgeNumber; // suppress warning
    // One can cast from PPBallLarusNode to BLInstrumentationNode since createNode
    // is overriden to produce BLInstrumentationNode.
    return( new BLInstrumentationEdge((BLInstrumentationNode*)source,
                                      (BLInstrumentationNode*)target) );
}

// Sets the Value corresponding to the pathNumber register, constant,
// or phinode.  Used by the instrumentation code to remember path
// number Values.
Value* BLInstrumentationNode::getStartingPathNumber(){
    return(_startingPathNumber);
}

// Sets the Value of the pathNumber.  Used by the instrumentation code.
void BLInstrumentationNode::setStartingPathNumber(Value* pathNumber) {
    DEBUG(dbgs() << "  SPN-" << getName() << " <-- " << (pathNumber ?
                                                         pathNumber->getName() :
                                                         "unused") << '\n');
    _startingPathNumber = pathNumber;
}

Value* BLInstrumentationNode::getEndingPathNumber(){
    return(_endingPathNumber);
}

void BLInstrumentationNode::setEndingPathNumber(Value* pathNumber) {
    DEBUG(dbgs() << "  EPN-" << getName() << " <-- "
                 << (pathNumber ? pathNumber->getName() : "unused") << '\n');
    _endingPathNumber = pathNumber;
}

// Get the PHINode Instruction for this node.  Used by instrumentation
// code.
PHINode* BLInstrumentationNode::getPathPHI() {
    return(_pathPHI);
}

// Set the PHINode Instruction for this node.  Used by instrumentation
// code.
void BLInstrumentationNode::setPathPHI(PHINode* pathPHI) {
    _pathPHI = pathPHI;
}

// Get the unique ID of the node
unsigned int BLInstrumentationNode::getNodeId() {
    return(_blockId);
}

// Removes the edge from the appropriate predecessor and successor
// lists.
void BLInstrumentationDag::unlinkEdge(PPBallLarusEdge* edge) {
    if(edge == getExitRootEdge())
        DEBUG(dbgs() << " Removing exit->root edge\n");

    edge->getSource()->removeSuccEdge(edge);
    edge->getTarget()->removePredEdge(edge);
}

// Makes an edge part of the spanning tree.
void BLInstrumentationDag::makeEdgeSpanning(BLInstrumentationEdge* edge) {
    edge->setIsInSpanningTree(true);
    _treeEdges.push_back(edge);
}

// Pushes initialization and calls itself recursively.
void BLInstrumentationDag::pushInitializationFromEdge(
        BLInstrumentationEdge* edge) {
    PPBallLarusNode* target;

    target = edge->getTarget();
    if( target->getNumberPredEdges() > 1 || target == getExit() ) {
        return;
    } else {
        for(PPBLEdgeIterator next = target->succBegin(),
                    end = target->succEnd(); next != end; next++) {
            BLInstrumentationEdge* intoEdge = (BLInstrumentationEdge*) *next;

            // Skip split edges
            if (intoEdge->getType() == PPBallLarusEdge::SPLITEDGE)
                continue;

            long increment = intoEdge->getIncrement() + edge->getIncrement();
            if(increment < 0)
                this->_errorNegativeIncrements = true;
            intoEdge->setIncrement(increment);
            intoEdge->setIsInitialization(true);
            pushInitializationFromEdge(intoEdge);
        }

        edge->setIncrement(0);
        edge->setIsInitialization(false);
    }
}

// Pushes path counter increments up recursively.
void BLInstrumentationDag::pushCountersFromEdge(BLInstrumentationEdge* edge) {
    PPBallLarusNode* source;

    source = edge->getSource();
    if(source->getNumberSuccEdges() > 1 || source == getRoot()
       || edge->isInitialization()) {
        return;
    } else {
        for(PPBLEdgeIterator previous = source->predBegin(),
                    end = source->predEnd(); previous != end; previous++) {
            BLInstrumentationEdge* fromEdge = (BLInstrumentationEdge*) *previous;

            // Skip split edges
            if (fromEdge->getType() == PPBallLarusEdge::SPLITEDGE)
                continue;

            long increment = fromEdge->getIncrement() + edge->getIncrement();
            if(increment < 0)
                this->_errorNegativeIncrements = true;
            fromEdge->setIncrement(increment);
            fromEdge->setIsCounterIncrement(true);
            pushCountersFromEdge(fromEdge);
        }

        edge->setIncrement(0);
        edge->setIsCounterIncrement(false);
    }
}

// Depth first algorithm for determining the chord increments.
void BLInstrumentationDag::calculateChordIncrementsDfs(long weight,
                                                       PPBallLarusNode* v, PPBallLarusEdge* e) {
    BLInstrumentationEdge* f;

    for(PPBLEdgeIterator treeEdge = _treeEdges.begin(),
                end = _treeEdges.end(); treeEdge != end; treeEdge++) {
        f = (BLInstrumentationEdge*) *treeEdge;
        if(e != f && v == f->getTarget()) {
            calculateChordIncrementsDfs(
                    calculateChordIncrementsDir(e,f)*(weight) +
                    f->getWeight(), f->getSource(), f);
        }
        if(e != f && v == f->getSource()) {
            calculateChordIncrementsDfs(
                    calculateChordIncrementsDir(e,f)*(weight) +
                    f->getWeight(), f->getTarget(), f);
        }
    }

    for(PPBLEdgeIterator chordEdge = _chordEdges.begin(),
                end = _chordEdges.end(); chordEdge != end; chordEdge++) {
        f = (BLInstrumentationEdge*) *chordEdge;
        if(v == f->getSource() || v == f->getTarget()) {
            long increment = f->getIncrement() +
                             calculateChordIncrementsDir(e,f)*weight;
            if(increment < 0)
                this->_errorNegativeIncrements = true;
            f->setIncrement(increment);
        }
    }
}

// Determines the relative direction of two edges.
int BLInstrumentationDag::calculateChordIncrementsDir(PPBallLarusEdge* e,
                                                      PPBallLarusEdge* f) {
    if( e == NULL)
        return(1);
    else if(e->getSource() == f->getTarget()
            || e->getTarget() == f->getSource())
        return(1);

    return(-1);
}

bool BLInstrumentationDag::error_negativeIncrements(){
    return this->_errorNegativeIncrements;
}


#include <vector>
#include <sstream>
#include <set>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "BBProfiling/BBProfiling.h"

using namespace std;

static RegisterPass<BBProfiling> X("bb-profiling", "profiling executed basic blocks", true, true);


void getExitBlock(Function *pFunc, vector<BasicBlock *> &exitBlocks) {
    exitBlocks.clear();

    for (Function::iterator itBB = pFunc->begin(); itBB != pFunc->end(); itBB++) {
        BasicBlock *BB = &*itBB;

        if (isa<ReturnInst>(BB->getTerminator())) {
            exitBlocks.push_back(BB);
        }
    }

    assert(exitBlocks.size() == 1);
}


BasicBlock *BBProfilingNode::getBlock() {
    return _basicBlock;
}

BBProfilingNode::NodeColor BBProfilingNode::getColor() {
    return _color;
}

void BBProfilingNode::setColor(BBProfilingNode::NodeColor color) {
    _color = color;
}


BBPFEdgeIterator BBProfilingNode::predBegin() {
    return _predEdges.begin();
}

BBPFEdgeIterator BBProfilingNode::predEnd() {
    return _predEdges.end();
}

unsigned BBProfilingNode::getNumberPredEdges() {
    return _predEdges.size();
}

BBPFEdgeIterator BBProfilingNode::succBegin() {
    return _succEdges.begin();
}

BBPFEdgeIterator BBProfilingNode::succEnd() {
    return _succEdges.end();
}

unsigned BBProfilingNode::getNumberSuccEdges() {
    return _succEdges.size();
}


void BBProfilingNode::addPredEdge(BBProfilingEdge *edge) {
    _predEdges.push_back(edge);
}


void BBProfilingNode::removePredEdge(BBProfilingEdge *edge) {
    removeEdge(_predEdges, edge);
}

void BBProfilingNode::clearPredEdge() {
    _predEdges.clear();
}

void BBProfilingNode::addSuccEdge(BBProfilingEdge *edge) {
    _succEdges.push_back(edge);
}


void BBProfilingNode::removeSuccEdge(BBProfilingEdge *edge) {
    removeEdge(_succEdges, edge);
}

void BBProfilingNode::clearSuccEdge() {
    _succEdges.clear();
}


string BBProfilingNode::getName() {
    stringstream name;

    if (getBlock() != NULL) {
        if (getBlock()->hasName()) {
            string tempName(getBlock()->getName());
            name << tempName.c_str() << " (" << _uid << ')';
        } else {
            name << "<unnamed> (" << _uid << ')';
        }
    } else {
        name << "<null> (" << _uid << ')';
    }

    return name.str();
}


void BBProfilingNode::removeEdge(BBPFEdgeVector &v, BBProfilingEdge *e) {
    for (BBPFEdgeIterator i = v.begin(), end = v.end(); i != end; i++) {
        if ((*i) == e) {
            v.erase(i);
            break;
        }
    }
}


BBProfilingNode *BBProfilingEdge::getSource() const {
    return _source;
}

void BBProfilingEdge::setSource(BBProfilingNode *s) {
    _source = s;
}

BBProfilingNode *BBProfilingEdge::getTarget() const {
    return _target;
}

void BBProfilingEdge::setTarget(BBProfilingNode *t) {
    _target = t;
}

BBProfilingEdge::EdgeType BBProfilingEdge::getType() const {
    return _edgeType;
}

void BBProfilingEdge::setType(BBProfilingEdge::EdgeType type) {
    _edgeType = type;
}

unsigned long BBProfilingEdge::getWeight() {
    return _weight;
}

void BBProfilingEdge::setWeight(unsigned long weight) {
    _weight = weight;
}

long BBProfilingEdge::getIncrement() {
    return _increment;
}


void BBProfilingEdge::setIncrement(long inc) {
    _increment = inc;
}

unsigned BBProfilingEdge::getDuplicateNumber() {
    return _duplicateNumber;
}


void BBProfilingGraph::init() {
    //BBPFBlockNodeMap inGraph;
    stack<BBProfilingNode *> dfsStack;

    _root = addNode(&(_function.getEntryBlock()));
    _exit = addNode(NULL);

    dfsStack.push(getRoot());

    while (dfsStack.size()) {
        BBProfilingNode *currentNode = dfsStack.top();
        BasicBlock *currentBlock = currentNode->getBlock();

        if (currentNode->getColor() != BBProfilingNode::WHITE) {
            dfsStack.pop();
            currentNode->setColor(BBProfilingNode::BLACK);
        } else {
            TerminatorInst *terminator = currentBlock->getTerminator();
            if (isa<ReturnInst>(terminator) || isa<UnreachableInst>(terminator) || isa<ResumeInst>(terminator)) {
                addEdge(currentNode, getExit(), 0);
            }

            currentNode->setColor(BBProfilingNode::GRAY);
            _inGraphMapping[currentBlock] = currentNode;

            BasicBlock *oldSuccessor = 0;
            unsigned duplicateNumber = 0;

            for (succ_iterator successor = succ_begin(currentBlock), succEnd = succ_end(currentBlock);
                 successor != succEnd; oldSuccessor = *successor, ++successor) {
                BasicBlock *succBB = *successor;

                if (oldSuccessor == succBB) {
                    duplicateNumber++;
                } else {
                    duplicateNumber = 0;
                }

                BBProfilingNode *succNode = _inGraphMapping[succBB];

                if (succNode && succNode->getColor() == BBProfilingNode::BLACK) {
                    addEdge(currentNode, succNode, duplicateNumber);
                } else if (succNode && succNode->getColor() == BBProfilingNode::GRAY) {
                    //backedge
                    addEdge(currentNode, succNode, duplicateNumber);
                } else {
                    BBProfilingNode *childNode;

                    if (succNode) {
                        childNode = succNode;
                    } else {
                        childNode = addNode(succBB);
                        _inGraphMapping[succBB] = childNode;
                    }

                    addEdge(currentNode, childNode, duplicateNumber);
                    dfsStack.push(childNode);
                }

            }
        }
    }

    addEdge(getExit(), getRoot(), 0);
}

BBProfilingNode *BBProfilingGraph::createNode(BasicBlock *BB) {
    return new BBProfilingNode(BB);
}

BBProfilingNode *BBProfilingGraph::addNode(BasicBlock *BB) {
    BBProfilingNode *newNode = createNode(BB);
    _nodes.push_back(newNode);

    return newNode;

}


BBProfilingEdge *
BBProfilingGraph::createEdge(BBProfilingNode *source, BBProfilingNode *target, unsigned duplicateCount) {
    return new BBProfilingEdge(source, target, duplicateCount);
}

BBProfilingEdge *BBProfilingGraph::addEdge(BBProfilingNode *source, BBProfilingNode *target, unsigned duplicateCount) {
    BBProfilingEdge *newEdge = createEdge(source, target, duplicateCount);
    _edges.push_back(newEdge);
    source->addSuccEdge(newEdge);
    target->addPredEdge(newEdge);
    return newEdge;
}

unsigned BBProfilingEdge::getSuccessorNumber() {
    BBProfilingNode *sourceNode = getSource();
    BBProfilingNode *targetNode = getTarget();

    BasicBlock *source = sourceNode->getBlock();
    BasicBlock *target = targetNode->getBlock();

    if (sourceNode == NULL || target == NULL) {
        return 0;
    }

    TerminatorInst *terminator = source->getTerminator();

    unsigned i;

    for (i = 0; i < terminator->getNumSuccessors(); i++) {
        if (terminator->getSuccessor(i) == target) {
            break;
        }
    }

    return i;
}

BBProfilingGraph::~BBProfilingGraph() {
    for (BBPFEdgeIterator edge = _edges.begin(), end = _edges.end(); edge != end; ++edge) {
        delete (*edge);
    }

    for (BBPFNodeIterator node = _nodes.begin(), end = _nodes.end(); node != end; ++node) {
        delete (*node);
    }
}


BBProfilingNode *BBProfilingGraph::getRoot() {
    return _root;
}

BBProfilingNode *BBProfilingGraph::getExit() {
    return _exit;
}

Function &BBProfilingGraph::getFunction() {
    return _function;
}

unsigned BBProfilingGraph::getEdgeNum() {
    return _edges.size();
}


unsigned BBProfilingGraph::getNodeNum() {
    return _nodes.size();
}

void BBProfilingGraph::printNodeEdgeInfo() {
    for (BBPFNodeIterator itNode = _nodes.begin(); itNode != _nodes.end(); itNode++) {
        errs() << (*itNode)->getName() << "-> {";

        for (BBPFEdgeIterator itEdge = (*itNode)->succBegin(); itEdge != (*itNode)->succEnd(); itEdge++) {
            errs() << (*itEdge)->getTarget()->getName() << ", ";
        }

        errs() << "}\n";
    }


    errs() << "\n\n";

    for (BBPFEdgeIterator itEdge = _edges.begin(); itEdge != _edges.end(); itEdge++) {
        errs() << (*itEdge)->getSource()->getName() << "->" << (*itEdge)->getTarget()->getName() << " "
               << (*itEdge)->getWeight() << " " << (*itEdge)->getIncrement() << "\n";
    }
}

void BBProfilingGraph::splitNotExitBlock() {
    unsigned index = 0;
    unsigned currentSize = _nodes.size();

    for (; index < currentSize; index++) {
        if (_nodes[index]->getBlock() == NULL) {
            continue;
        }

        BBProfilingNode *currentNode = _nodes[index];
        BasicBlock *currentBlock = currentNode->getBlock();
        BBProfilingNode *newNode = addNode(currentBlock);

        for (BBPFEdgeIterator edgeBegin = currentNode->succBegin(); edgeBegin != currentNode->succEnd(); edgeBegin++) {
            newNode->addSuccEdge(*edgeBegin);
            (*edgeBegin)->setSource(newNode);
        }

        currentNode->clearSuccEdge();

        BBProfilingEdge *edge = addEdge(currentNode, newNode, 0);
        edge->setType(BBProfilingEdge::BB_PHONY);
        edge->setWeight(1);
    }
}


void BBProfilingGraph::calculateSpanningTree() {
    map<BBProfilingNode *, unsigned> C;
    map<BBProfilingNode *, BBProfilingEdge *> E;
    set<BBProfilingNode *> FNode;
    set<BBProfilingEdge *> FEdge;
    set<BBProfilingNode *> Q;


    for (BBPFNodeIterator itNode = _nodes.begin(); itNode != _nodes.end(); itNode++) {
        C[*itNode] = 100000;
        E[*itNode] = NULL;
        Q.insert(*itNode);
    }


    while (Q.size() > 0) {
        set<BBProfilingNode *>::iterator itSetBegin = Q.begin();
        set<BBProfilingNode *>::iterator itSetEnd = Q.end();

        unsigned minC = C[*itSetBegin];
        BBProfilingNode *minNode = *itSetBegin;

        while (itSetBegin != itSetEnd) {
            if (C[*itSetBegin] < minC) {
                minC = C[*itSetBegin];
                minNode = *itSetBegin;
            }

            itSetBegin++;
        }

        Q.erase(minNode);

        FNode.insert(minNode);

        if (E[minNode] != NULL) {
            FEdge.insert(E[minNode]);
        }


        for (BBPFEdgeIterator itEdgeBegin = minNode->predBegin(); itEdgeBegin != minNode->predEnd(); itEdgeBegin++) {
            BBProfilingNode *w = (*itEdgeBegin)->getSource();

            if (Q.find(w) != Q.end()) {
                if ((*itEdgeBegin)->getWeight() < C[w]) {
                    C[w] = (*itEdgeBegin)->getWeight();
                    E[w] = *itEdgeBegin;
                }
            }
        }

        for (BBPFEdgeIterator itEdgeBegin = minNode->succBegin(); itEdgeBegin != minNode->succEnd(); itEdgeBegin++) {
            BBProfilingNode *w = (*itEdgeBegin)->getTarget();

            if (Q.find(w) != Q.end()) {
                if ((*itEdgeBegin)->getWeight() < C[w]) {
                    C[w] = (*itEdgeBegin)->getWeight();
                    E[w] = *itEdgeBegin;
                }
            }
        }
    }

    for (BBPFEdgeIterator itEdge = _edges.begin(); itEdge != _edges.end(); itEdge++) {
        if (FEdge.find(*itEdge) != FEdge.end()) {
            _treeEdges.push_back(*itEdge);
        } else {
            _chords.push_back(*itEdge);
        }
    }

    assert(_chords.size() + _treeEdges.size() == _edges.size());

/*
	for(BBPFEdgeIterator itEdge = _treeEdges.begin(); itEdge != _treeEdges.end(); itEdge ++ )
	{
		errs() << (*itEdge)->getSource()->getName() << "->" << (*itEdge)->getTarget()->getName() << "\n";
	}

	errs() << "\n\n";


	for(BBPFEdgeIterator itEdge = _chords.begin(); itEdge != _chords.end(); itEdge ++ )
	{
		errs() << (*itEdge)->getSource()->getName() << "->" << (*itEdge)->getTarget()->getName() << "\n";
	}

*/
/*
	set<BBProfilingEdge * >::iterator itSetBegin = FEdge.begin();
	set<BBProfilingEdge * >::iterator itSetEnd   = FEdge.end();

	while(itSetBegin != itSetEnd)
	{
		errs() << (*itSetBegin)->getSource()->getName() << "->" << (*itSetBegin)->getTarget()->getName() << "\n";

		_chords.insert(*itSetBegin);
		itSetBegin++;
	}
*/
}

void BBProfilingGraph::calculateChordIncrements() {
    calculateChordIncrementsDfs(0, getRoot(), NULL);

    BBProfilingEdge *chord;

    for (BBPFEdgeIterator chordEdge = _chords.begin(), end = _chords.end(); chordEdge != end; chordEdge++) {
        chord = *chordEdge;
        long inc = chord->getIncrement() + chord->getWeight();

        if (inc < 0) {
            _errorNegativeIncrements = true;
        }

        chord->setIncrement(inc);
    }
}


void BBProfilingGraph::calculateChordIncrementsDfs(long weight, BBProfilingNode *v, BBProfilingEdge *e) {
    BBProfilingEdge *f;

    for (BBPFEdgeIterator treeEdge = _treeEdges.begin(), end = _treeEdges.end(); treeEdge != end; treeEdge++) {
        f = *treeEdge;

        if (e != f && v == f->getTarget()) {
            calculateChordIncrementsDfs(calculateChordIncrementsDir(e, f) * weight + f->getWeight(), f->getSource(), f);
        }

        if (e != f && v == f->getSource()) {
            calculateChordIncrementsDfs(calculateChordIncrementsDir(e, f) * weight + f->getWeight(), f->getTarget(), f);
        }
    }

    for (BBPFEdgeIterator chordEdge = _chords.begin(), end = _chords.end(); chordEdge != end; chordEdge++) {
        f = *chordEdge;

        if (v == f->getSource() || v == f->getTarget()) {
            long inc = f->getIncrement() + calculateChordIncrementsDir(e, f) * weight;

            if (inc < 0) {
                this->_errorNegativeIncrements = true;
            }

            f->setIncrement(inc);
        }
    }

}


int BBProfilingGraph::calculateChordIncrementsDir(BBProfilingEdge *e, BBProfilingEdge *f) {
    if (e == NULL) {
        return 1;
    } else if (e->getSource() == f->getTarget() || e->getTarget() == f->getSource()) {
        return 1;
    }

    return -1;
}

BBProfilingEdge *BBProfilingGraph::addQueryChord() {
    vector<BasicBlock *> vecExitBlocks;
    getExitBlock(&_function, vecExitBlocks);

    BBProfilingNode *pExit = _inGraphMapping[vecExitBlocks[0]];
    //errs() << pExit->getNumberSuccEdges() << "\n";
    BBProfilingNode *pNewExit = (*(pExit->succBegin()))->getTarget();

    BBProfilingEdge *pQueryEdge = addEdge(pNewExit, getRoot(), 0);
    pQueryEdge->setType(BBProfilingEdge::QUERY_PHONY);

    _chords.push_back(pQueryEdge);

    return pQueryEdge;
}

void BBProfilingGraph::instrumentLocalCounterUpdate(AllocaInst *numLocalCounter, GlobalVariable *numCost) {
    vector<BasicBlock *> vecExitBlocks;
    getExitBlock(&_function, vecExitBlocks);

    BasicBlock *pExitBlock = vecExitBlocks[0];

    long numExitUpdates = 0;

    for (BBPFEdgeIterator itEdge = _chords.begin(); itEdge != _chords.end(); itEdge++) {
        BBProfilingEdge *pEdge = *itEdge;

        if (pEdge->getIncrement() == 0) {
            continue;
        }

        if (pEdge->getType() == BBProfilingEdge::BB_PHONY) {
            if (pEdge->getTarget()->getBlock() == pExitBlock) {
                numExitUpdates += pEdge->getIncrement();
            } else {
                BasicBlock *pBlock = pEdge->getTarget()->getBlock();
                Instruction *pTerm = pBlock->getTerminator();

                LoadInst *pLoadLocalCost = new LoadInst(numLocalCounter, "", false, pTerm);
                pLoadLocalCost->setAlignment(8);
                BinaryOperator *pAdd = BinaryOperator::Create(Instruction::Add, pLoadLocalCost, ConstantInt::get(
                        IntegerType::get(_function.getParent()->getContext(), 64), pEdge->getIncrement()), "add",
                                                              pTerm);
                StoreInst *pStore = new StoreInst(pAdd, numLocalCounter, false, pTerm);
                pStore->setAlignment(8);
            }
        } else if (pEdge->getType() == BBProfilingEdge::NORMAL) {
            BasicBlock *pBlock = NULL;

            BBProfilingNode *sourceNode = pEdge->getSource();
            BBProfilingNode *targetNode = pEdge->getTarget();

            BasicBlock *sourceBlock = sourceNode->getBlock();
            BasicBlock *targetBlock = targetNode->getBlock();

            if (sourceNode->getNumberSuccEdges() == 1) {
                pBlock = sourceBlock;
            } else if (targetNode->getNumberPredEdges() == 1) {
                pBlock = targetBlock;
            } else {
                unsigned succNum = pEdge->getSuccessorNumber();
                TerminatorInst *terminator = sourceBlock->getTerminator();

                if (SplitCriticalEdge(terminator, succNum)) {
                    pBlock = terminator->getSuccessor(succNum);
                } else {
                    assert(false);
                }

            }

            Instruction *pTerm = pBlock->getTerminator();

            LoadInst *pLoadLocalCost = new LoadInst(numLocalCounter, "", false, pTerm);
            pLoadLocalCost->setAlignment(8);
            BinaryOperator *pAdd = BinaryOperator::Create(Instruction::Add, pLoadLocalCost, ConstantInt::get(
                    IntegerType::get(_function.getParent()->getContext(), 64), numExitUpdates), "add", pTerm);
            StoreInst *pStore = new StoreInst(pAdd, numLocalCounter, false, pTerm);
            pStore->setAlignment(8);
        } else if (pEdge->getType() == BBProfilingEdge::QUERY_PHONY) {
            numExitUpdates += pEdge->getIncrement();
        }

    }


    if (numExitUpdates != 0) {
        TerminatorInst *pTerm = pExitBlock->getTerminator();

        LoadInst *pLoadLocalCost = new LoadInst(numLocalCounter, "", false, pTerm);
        pLoadLocalCost->setAlignment(8);
        BinaryOperator *pAdd = BinaryOperator::Create(Instruction::Add, pLoadLocalCost, ConstantInt::get(
                IntegerType::get(_function.getParent()->getContext(), 64), numExitUpdates), "add", pTerm);

        LoadInst *pLoadBBCost = new LoadInst(numCost, "", false, pTerm);
        pLoadBBCost->setAlignment(8);

        pAdd = BinaryOperator::Create(Instruction::Add, pLoadBBCost, pAdd, "add", pTerm);
        StoreInst *pStore = new StoreInst(pAdd, numCost, false, pTerm);
        pStore->setAlignment(8);

    } else {
        TerminatorInst *pTerm = pExitBlock->getTerminator();
        LoadInst *pLoadLocalCost = new LoadInst(numLocalCounter, "", false, pTerm);
        pLoadLocalCost->setAlignment(8);
        LoadInst *pLoadBBCost = new LoadInst(numCost, "", false, pTerm);
        pLoadBBCost->setAlignment(8);
        BinaryOperator *pAdd = BinaryOperator::Create(Instruction::Add, pLoadLocalCost, pLoadBBCost, "add", pTerm);
        StoreInst *pStore = new StoreInst(pAdd, numCost, false, pTerm);
        pStore->setAlignment(8);
    }
}


char BBProfiling::ID = 0;

void BBProfiling::getAnalysisUsage(AnalysisUsage &AU) const {

}

BBProfiling::BBProfiling() : ModulePass(ID) {

}

void BBProfiling::SetupTypes(Module *pModule) {
    this->VoidType = Type::getVoidTy(pModule->getContext());
    this->LongType = IntegerType::get(pModule->getContext(), 64);
    this->IntType = IntegerType::get(pModule->getContext(), 32);
}

void BBProfiling::SetupGlobals(Module *pModule) {
    assert(pModule->getGlobalVariable("numCost") == NULL);
    this->numCost = new GlobalVariable(*pModule, this->LongType, false, GlobalValue::ExternalLinkage, 0, "numCost");
    this->numCost->setAlignment(8);
    this->numCost->setInitializer(ConstantInt::get(this->LongType, 0));
}

void BBProfiling::SetupHooks(Module *pModule) {
    this->PrintExecutionCost = pModule->getFunction("PrintExecutionCost");

    if (!this->PrintExecutionCost) {
        vector<Type *> ArgTypes;
        ArgTypes.push_back(this->LongType);
        FunctionType *PrintExecutionCostType = FunctionType::get(this->VoidType, ArgTypes, false);


        this->PrintExecutionCost = Function::Create(PrintExecutionCostType, GlobalValue::ExternalLinkage,
                                                    "PrintExecutionCost", pModule);
        ArgTypes.clear();
    }

}


bool BBProfiling::runOnFunction(Function &F) {
    if (F.getName() == "JS_Assert") {
        return false;
    }

    BBProfilingGraph bbGraph = BBProfilingGraph(F);
    bbGraph.init();
    bbGraph.splitNotExitBlock();
    //bbGraph.printNodeEdgeInfo();

    //bbGraph.printNodeEdgeInfo();
    bbGraph.calculateSpanningTree();

    BBProfilingEdge *pQueryEdge = bbGraph.addQueryChord();
    bbGraph.calculateChordIncrements();
    //bbGraph.printNodeEdgeInfo();

    Instruction *entryInst = F.getEntryBlock().getFirstNonPHI();
    AllocaInst *numLocalCounter = new AllocaInst(this->LongType, 0, "LOCAL_COST_BB", entryInst);
    numLocalCounter->setAlignment(8);
    StoreInst *pStore = new StoreInst(ConstantInt::get(this->LongType, 0), numLocalCounter, false, entryInst);
    pStore->setAlignment(8);

    bbGraph.instrumentLocalCounterUpdate(numLocalCounter, this->numCost);

    return true;
}

void BBProfiling::InstrumentResultDumper(Function *pMain) {
    LoadInst *pLoad = NULL;

    for (Function::iterator BI = pMain->begin(); BI != pMain->end(); ++BI) {
        for (BasicBlock::iterator II = BI->begin(); II != BI->end(); ++II) {
            Instruction *Ins = &*II;

            if (isa<ReturnInst>(Ins)) {
                vector<Value *> vecParams;
                pLoad = new LoadInst(this->numCost, "", false, Ins);
                pLoad->setAlignment(8);
                vecParams.push_back(pLoad);

                this->PrintExecutionCost->dump();

                CallInst *pCall = CallInst::Create(this->PrintExecutionCost, vecParams, "", Ins);
                pCall->setCallingConv(CallingConv::C);
                pCall->setTailCall(false);
                AttributeList aSet;
                pCall->setAttributes(aSet);

            }
        }
    }
}

bool BBProfiling::runOnModule(Module &M) {

    SetupTypes(&M);
    SetupGlobals(&M);
    SetupHooks(&M);


    bool changed = false;


    for (Module::iterator FI = M.begin(); FI != M.end(); FI++) {
        if (FI->begin() == FI->end()) {
            continue;
        }

        changed |= runOnFunction(*FI);
    }


    Function *pMain = M.getFunction("main");
    InstrumentResultDumper(pMain);


    return changed;
}
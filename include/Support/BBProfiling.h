#include <map>
#include <set>
#include <stack>
#include <vector>

#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/raw_ostream.h"


using namespace llvm;
using namespace std;

class BBProfilingNode;

class BBProfilingEdge;

class BBProfilingGraph;

typedef vector<BBProfilingNode *> BBPFNodeVector;
typedef vector<BBProfilingNode *>::iterator BBPFNodeIterator;
typedef vector<BBProfilingEdge *> BBPFEdgeVector;
typedef vector<BBProfilingEdge *>::iterator BBPFEdgeIterator;
typedef map<BasicBlock *, BBProfilingNode *> BBPFBlockNodeMap;
typedef stack<BBProfilingNode *> BBPFNodeStack;
typedef set<BBProfilingEdge *> BBPFEdgeSet;

void getExitBlock(Function *pFunc, vector<BasicBlock *> &exitBlocks);


class BBProfilingNode {
public:
    enum NodeColor {
        WHITE, GRAY, BLACK
    };

    BBProfilingNode(BasicBlock *BB) : _basicBlock(BB), _color(WHITE) {
        static unsigned nextUID = 0;
        _uid = nextUID++;
    }

    BasicBlock *getBlock();

    NodeColor getColor();

    void setColor(NodeColor color);

    BBPFEdgeIterator succBegin();

    BBPFEdgeIterator succEnd();

    BBPFEdgeIterator predBegin();

    BBPFEdgeIterator predEnd();


    unsigned getNumberSuccEdges();

    unsigned getNumberPredEdges();

    void addPredEdge(BBProfilingEdge *edge);

    void removePredEdge(BBProfilingEdge *edge);

    void clearPredEdge();

    void addSuccEdge(BBProfilingEdge *edge);

    void removeSuccEdge(BBProfilingEdge *edge);

    void clearSuccEdge();


    string getName();

private:
    BasicBlock *_basicBlock;
    BBPFEdgeVector _predEdges;
    BBPFEdgeVector _succEdges;
    NodeColor _color;
    unsigned _uid;

    void removeEdge(BBPFEdgeVector &v, BBProfilingEdge *e);

};


class BBProfilingEdge {
public:
    enum EdgeType {
        NORMAL, BACKEDGE, SPLITEDGE,
        BACKEDGE_PHONY, SPLITEDGE_PHONY, CALLEDGE_PHONY,
        BB_PHONY, EXIT_PHONY, QUERY_PHONY
    };

    BBProfilingEdge(BBProfilingNode *source, BBProfilingNode *target, unsigned duplicateNumber) : _source(source),
                                                                                                  _target(target),
                                                                                                  _weight(0),
                                                                                                  _increment(0),
                                                                                                  _edgeType(NORMAL),
                                                                                                  _duplicateNumber(
                                                                                                          duplicateNumber) {}

    BBProfilingNode *getSource() const;

    void setSource(BBProfilingNode *);

    BBProfilingNode *getTarget() const;

    void setTarget(BBProfilingNode *);

    EdgeType getType() const;

    void setType(EdgeType type);

    unsigned long getWeight();

    void setWeight(unsigned long weight);

    long getIncrement();

    void setIncrement(long inc);

    unsigned getDuplicateNumber();

    unsigned getSuccessorNumber();

private:
    BBProfilingNode *_source;
    BBProfilingNode *_target;
    unsigned long _weight;
    long _increment;
    EdgeType _edgeType;
    unsigned _duplicateNumber;
};

class BBProfilingGraph {
public:
    BBProfilingGraph(Function &F) : _root(NULL), _exit(NULL), _errorEdgeOverflow(false),
                                    _errorNegativeIncrements(false), _function(F) {}

    void init();

    virtual ~BBProfilingGraph();

    BBProfilingNode *getRoot();

    BBProfilingNode *getExit();

    Function &getFunction();

    BBProfilingNode *createNode(BasicBlock *BB);

    BBProfilingNode *addNode(BasicBlock *BB);

    BBProfilingEdge *createEdge(BBProfilingNode *source, BBProfilingNode *target, unsigned duplicateCount);

    BBProfilingEdge *addEdge(BBProfilingNode *source, BBProfilingNode *target, unsigned duplicateCount);


    void calculateChordIncrements();

    void calculateChordIncrementsDfs(long weight, BBProfilingNode *v, BBProfilingEdge *e);

    int calculateChordIncrementsDir(BBProfilingEdge *e, BBProfilingEdge *f);


    void splitNotExitBlock();

    void calculateSpanningTree();

    BBProfilingEdge *addQueryChord();

    void instrumentLocalCounterUpdate(AllocaInst *numLocalCounter);


    unsigned getEdgeNum();

    unsigned getNodeNum();

    void printNodeEdgeInfo();

private:

    BBPFNodeVector _nodes;
    BBPFEdgeVector _edges;


    BBPFEdgeVector _treeEdges;
    BBPFEdgeVector _chords;

    BBProfilingNode *_root;
    BBProfilingNode *_exit;

    BBPFBlockNodeMap _inGraphMapping;

    bool _errorEdgeOverflow;
    bool _errorNegativeIncrements;
    Function &_function;


};

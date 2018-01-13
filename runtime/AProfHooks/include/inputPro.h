#ifndef INIEXPOSIZE
#define INIEXPOSIZE 1024
#endif


typedef struct _ShadowStackElem {
    char *funcname; // function name of current activation
    unsigned long ts; // time stamp of
    unsigned long rms;
    unsigned long cost;
} ShadowStackElement;


typedef struct _ShadowStack {
    struct _ShadowStackElem *StackElements;
    unsigned long top; // ****extra attention, stackelemets[top-1] is the top element.
    unsigned long size; // current size of the stack
    unsigned long elementsNum; // the size for the next expand
} ShadowStack;


int pushStackElem(char *funcName, unsigned long ts);


int popStackElem();


int emptyStack();


ShadowStackElement *top();


int createShadowStack();


int expandStack();


void aprof_call(char *funcName);


void aprof_return();


void aprof_read(unsigned long variable_address, unsigned int length);


void aprof_write(unsigned long variable_address, unsigned int length);


void aprof_collect(); //cost collection to be finished


unsigned long aprof_get_cost();


void aprof_increment_cost();
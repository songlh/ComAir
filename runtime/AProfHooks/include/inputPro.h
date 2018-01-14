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
    unsigned long top; // ****extra attention, stack elements[top-1] is the top element.
    unsigned long size; // current size of the stack
    unsigned long elementsNum; // the size for the next expand
} ShadowStack;


int pushStackElem(char *funcName);


int popStackElem();


int emptyStack();


ShadowStackElement *top();


int createShadowStack();


int expandStack();


void aprof_call(char *funcName);


void aprof_return();


void _aprof_read(void *memory_addr, unsigned int length);


void _aprof_write(void *memory_addr, unsigned int length);


void aprof_collect(); //cost collection to be finished


unsigned long aprof_get_cost();


void _aprof_increment_cost();

void update_call_cost();

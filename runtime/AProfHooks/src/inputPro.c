#include "common.h"
#include "hashMap.h"
#include "logger.h"
#include "inputPro.h"

#include <stdio.h>
#include <stdlib.h>

/* global variables */

ShadowStack *psStack;

unsigned long count = 0;

unsigned long bb_count = 0;

/* end */


int createShadowStack() {

    psStack = (ShadowStack *) malloc(sizeof(ShadowStack));
    psStack->StackElements = (struct _ShadowStackElem *) calloc(
            INIEXPOSIZE, sizeof(struct _ShadowStackElem));

    if (!psStack || !psStack->StackElements) {

        log_error("Error in createShadowStack(), memory allocation not success.");
        return STATUS_ERROR;

    } else {

        psStack->top = 0; // stack starts from 0, top is unsigned long type
        psStack->size = INIEXPOSIZE; // INIEXPOSIZE is the initial size of the stack.
        psStack->elementsNum = 0; // the initial elementnum is 0.
        return STATUS_SUCCESS;
    }
}


int expandStack() {

    // each expand we increase psStack->exposize elements
    ShadowStackElement *new_elements;
    long unsigned i;
    long unsigned new_stack_size;
    long unsigned current_stack_size;

    if (psStack == NULL) {
        log_error("expand stack(): shadow stack is NULL.");
        return -1;
    }

    current_stack_size = psStack->size;
    new_stack_size = current_stack_size * 2;
    new_elements = (struct _ShadowStackElem *) calloc(
            new_stack_size, sizeof(struct _ShadowStackElem));

    if (new_elements == NULL) {

        log_error("expand stack(): memory allocation not success.");
        return STATUS_NULL;

    } else {

        // expand psStack->stackelements with newstacksize size of stack elements
        for (i = 0; i < current_stack_size; i++) { // elements copy from old to new
            new_elements[i] = psStack->StackElements[i];
        }

        free(psStack->StackElements); // free the memory allocated previously
        psStack->StackElements = new_elements;
        psStack->size = new_stack_size;

        return STATUS_SUCCESS;
    }

}


ShadowStackElement *top() {

    if (psStack == NULL) {
        log_debug("top(): shadow stack is NULL.");
        return NULL;
    }

    if (psStack->top == 0) {

        log_debug("top(): stack is empty.");
        return NULL;

    } else {

        return &(psStack->StackElements[psStack->top - 1]);
    }
}


int emptyStack() {

    if (psStack == NULL) {

        log_error("empty stack(): stack pointer is NULL.");
        return STATUS_NULL;
    }

    if (psStack->top == 0)
        return 0;
    else
        return -2;
}


int pushStackElem(char *funcName) {

    int expand;

    if (psStack == NULL || funcName == NULL) {

        log_error("create Stack Node(): stack pointer or function name is NULL.");
        return STATUS_NULL;
    }

    if (psStack->top + 1 >= psStack->size) { // need to expand stack size;

        expand = expandStack();
        if (expand != STATUS_SUCCESS) {

            log_error("push stack elem(): expand stack() not succeed.");
            return STATUS_ERROR;
        }
    }
    log_trace("pushStackElem() current funcName: %s.", funcName);
    (psStack->StackElements[psStack->top]).funcname = funcName;
    // count is global count
    log_trace("pushStackElem() current count: %d.", count);
    (psStack->StackElements[psStack->top]).ts = count;
    (psStack->StackElements[psStack->top]).rms = 0;
    // init is current bb_count, it should update after call funcName!!!
    log_trace("pushStackElem() current bb_count: %ld", bb_count);
    (psStack->StackElements[psStack->top]).cost = bb_count;
    psStack->elementsNum++;
    psStack->top++;
    return STATUS_SUCCESS;

}


int popStackElem() {

    if (psStack == NULL) {

        log_error("popStackElem(): stack pointer is NULL.");
        return STATUS_NULL;

    }

    if (psStack->top == 0) {

        log_error("popStackElem(): stack is empty.");
        return STATUS_NULL;

    } else {

        psStack->top--;
        psStack->elementsNum--;
        return STATUS_SUCCESS;
    }
}


unsigned long aprof_get_cost() {
    // TODO: bb_count
    return bb_count - (psStack->StackElements[psStack->top]).cost;

}


void aprof_call(char *funcName) {

    count++;
    int push_result = pushStackElem(funcName);
    if (push_result != STATUS_SUCCESS) {
        log_error("execution call(): push stack error.");
        exit(0);
    }
    log_debug("aprof_call(), FuncName is %s, current stack size: %ld ",
              funcName, psStack->top);
}


void _aprof_collect() {

    if (psStack == NULL) {

        log_error("aprof_collect(): stack pointer is NULL.");
    }

    unsigned long stack_top = psStack->top - 1;
    log_fatal("Function Name %s; RMS %ld; Cost %ld",
              psStack->StackElements[stack_top].funcname,
              psStack->StackElements[stack_top].rms,
              psStack->StackElements[stack_top].cost);
}


void _aprof_return() {

    if (psStack == NULL) {

        log_error("aprof_return(): stack pointer is NULL.");
        return;
    }

    _aprof_collect();

    if (psStack->top > 1) {
        unsigned long stack_top = psStack->top - 1;
        // stack has at least two elements.
        psStack->StackElements[stack_top - 1].rms +=
                psStack->StackElements[stack_top].rms;
    }

    popStackElem();
}


void _aprof_read(void *memory_addr, unsigned int length) {

    char string_addr[30];
    unsigned long stack_top;
    unsigned int j = length; // j records byte length
    unsigned long startAddr = (unsigned long) memory_addr;

    for (unsigned long i = startAddr; j > 0; j--) {

        // turn i into hex string address, string_addr is w
        sprintf(string_addr, "%lx", i);

        // hash_map_result records the ts[string_addr]
        int hash_map_result = hashMapGet(string_addr);

        log_trace("aprof_read ts[%s], and the value is %d",
                  string_addr, hash_map_result);

        // stack_top should be the top of the stack
        stack_top = psStack->top - 1;

        //stack_result records the S[top].ts
        unsigned long stack_result = psStack->StackElements[stack_top].ts;
        log_trace("aprof_read S[top].ts is %d", stack_result);

        // ts[string_addr] < S[top].ts
        if (hash_map_result < stack_result) {

            psStack->StackElements[stack_top].rms++;

            // ts[w] != 0
            if (hash_map_result > 0) {

                // stack_top be the max index in S such that S[l].ts<=ts[string_addr]
                while (psStack->StackElements[stack_top].ts > hash_map_result && stack_top > 0) {
                    stack_top--;
                }

                if (stack_top <= 0) {
                    log_error("There is error in finding max index in Stack!");
                    exit(0);
                }

                //S[i].rms--
                psStack->StackElements[stack_top].rms--;
            }
        }

        // ts[string_addr] = count; count is global variable!
        hash_map_result = hashMapPut(string_addr, count);

        if (hash_map_result >= 0) {

            log_debug("read(): ts[%s] update successful.", string_addr);

        } else {

            log_error("read(): ts[%s] update failure.", string_addr);
        }

        // continue on next byte
        i++;
    }

}

void _aprof_write(void *memory_addr, unsigned int length) {

    unsigned long i;
    unsigned int j;
    long hash_map_result;
    char string_addr[30];
    unsigned long start_addr = (unsigned long) memory_addr;

    j = length; // j records byte length

    for (i = start_addr; j > 0; j--) {

        sprintf(string_addr, "%lx", i); // turn i into hex string address, string_addr is w
        hash_map_result = hashMapPut(string_addr, count); // ts[string_addr] = count;

        if (hash_map_result >= 0) {

            log_debug("write(): ts[%s] update successful.", string_addr);

        } else {

            log_error("write(): ts[start_adrr] update failure.", string_addr);
        }

        // FIXME::I think maybe should be i++?
        i++;
    }
}


void _aprof_increment_cost() {
    bb_count++;
}


void update_call_cost() {
    if (psStack->top > 0) {
        unsigned long stack_top = psStack->top - 1;

        if(stack_top == 0) {
            psStack->StackElements[stack_top].cost = bb_count;
        } else{
            psStack->StackElements[stack_top].cost =
                    bb_count - psStack->StackElements[stack_top].cost;
        }

        log_trace("update_call_cost(), current top: %ld ", psStack->top);
        log_trace("stack top funcName(): %s ", psStack->StackElements[stack_top].funcname);
        log_trace("update_call_cost(): %ld ", psStack->StackElements[stack_top].cost);
    }
}

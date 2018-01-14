#include <stdio.h>

#include "AProfHooks.h"


int aprof_init() {

    int r = createHashMap();
    if (r == 0) {
        printf("create hashMap error!");
        return STATUS_ERROR;
    }

    r = createShadowStack();
    if (r == 0) {
        printf("create shadow stack error!");
        return STATUS_ERROR;
    }

    return STATUS_SUCCESS;

}

void aprof_write(void *memory_addr, unsigned int length) {
    _aprof_write(memory_addr, length);
}

void aprof_read(void *memory_addr, unsigned int length) {
    _aprof_read(memory_addr, length);
}

void aprof_increment_cost() {
    _aprof_increment_cost();
}

void aprof_call_before(char *FuncName) {
    aprof_call(FuncName);
}

void aprof_call_after() {
    update_call_cost();
}


void PrintExecutionCost(long cost) {
    printf("TOTAL COST: %ld\n", cost);
}

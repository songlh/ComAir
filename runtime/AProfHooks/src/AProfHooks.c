#include <stdio.h>

#include "AProfHooks.h"


void logger_init() {
    FILE *fp = fopen(FILENAME, "w");
    log_set_level(LEVEL);
    log_set_fp(fp);
    log_set_quiet(QUIET);
}

int aprof_init() {

    logger_init();

    int r = createHashMap();
    if (r == 0) {
        log_error("create hashMap error!");
        return STATUS_ERROR;
    }

    r = createShadowStack();
    if (r == 0) {
        log_error("create shadow stack error!");
        return STATUS_ERROR;
    }
    log_debug("aprof init success!");
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

void aprof_return() {
    _aprof_return();
}

void PrintExecutionCost(long cost) {
    printf("TOTAL COST: %ld\n", cost);
}

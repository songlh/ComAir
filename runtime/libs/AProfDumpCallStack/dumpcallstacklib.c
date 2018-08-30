#include "dumpcallstacklib.h"


struct stack_elem shadow_stack[STACK_SIZE];
int stack_top = -1;
FILE *pFile;

void dump_init() {
    pFile = fopen ("dump-log.txt","a");
}

void enter_stack(int funcId) {
    stack_top++;
    shadow_stack[stack_top].funcId = funcId;
}


void outer_stack() {
    stack_top--;
}

void dump_stack() {
    int start = 0;
    char result[10];
    while (start <= stack_top) {
        // dump stack item to f
        sprintf(result, "%d", shadow_stack[start].funcId);
        fputs(result, pFile);
        fputs(",", pFile);
        start++;
    }
    fputs("\n", pFile);
}

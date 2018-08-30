#ifndef APROFHOOKS_LIBRARY_H
#define APROFHOOKS_LIBRARY_H

#include <stdlib.h>
#include <stdio.h>

#define STACK_SIZE 2000

struct stack_elem {
    int funcId; // function id
};


void dump_init();
void enter_stack();
void outer_stack();
void dump_stack();

#endif

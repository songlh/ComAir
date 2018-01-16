#ifndef COMAIR_STACK_H
#define COMAIR_STACK_H

#include <stdint.h>
#include <stddef.h>

struct stack_elem {
    int funcId; // function id
    unsigned long ts; // time stamp
    unsigned long rms;
    unsigned long cost;
    struct stack_elem *next;
};

struct stack {
    struct stack_elem *top;
};

#ifdef __cplusplus
extern "C" {


/*
 * Converts a pointer to the stack's element ptr into a pointer
 * to the structure that ptr is embedded inside. Supply the name
 * of the outer struct, 'type', and the member name of the stack
 * element 'member'. Taken from the Linux kernel container_of().
 */
#define stack_entry(ptr, type, member) ({ \
        const typeof( ((type *)0)->member ) *__mptr = (ptr); \
        (type *)( (char *)__mptr - offsetof(type,member) );})

void stack_init(struct stack *stack);
void push(struct stack *stack, struct stack_elem *elem);
struct stack_elem *pop(struct stack *);
struct stack_elem *top(struct stack *);
int size(struct stack *);
int isEmpty(struct stack *);

/*
 * Returns a stack with the cleared nodes contained.
 * You must iterate through the stack, convert the pointers
 * and free the pointers manually.
 */
struct stack clear(struct stack *);

};
#endif

#endif //COMAIR_STACK_H

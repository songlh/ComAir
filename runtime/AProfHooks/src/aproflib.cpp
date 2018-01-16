#include <vector>
#include <unordered_map>

#include "aproflib.h"
#include "logger.h"
#include "stack.h"

using namespace std;

unsigned long count = 0;
struct stack *shadow_stack = (struct stack *) malloc(
        sizeof(struct stack));

std::unordered_map<unsigned long, unsigned long> ts_map;

int aprof_init() {
    const char *FILENAME = "aprof_logger.txt";
    int LEVEL = 4;  // "TRACE" < "DEBUG" < "INFO" < "WARN" < "ERROR" < "FATAL"
    int QUIET = 1;
    FILE *fp = fopen(FILENAME, "w");
    log_init(fp, LEVEL, QUIET);
    stack_init(shadow_stack);

}

void aprof_write(void *memory_addr, unsigned int length) {
    unsigned long start_addr = (unsigned long) memory_addr;

    for (unsigned long i = start_addr; i < start_addr + length; i++) {
        ts_map[i] = count;
    }

    log_trace("aprof_write: memory_adrr is %ld, lenght is %ld, value is %ld",
              start_addr, length, count);

}

void aprof_read(void *memory_addr, unsigned int length) {
    unsigned long i;
    unsigned long start_addr = (unsigned long) memory_addr;
    struct stack_elem *topEle = top(shadow_stack);

    for (i = start_addr; i < (start_addr + length); i++) {

        // We assume that w has been wrote before reading.
        // ts[w] > 0 and ts[w] < S[top]
        if (ts_map[i] < topEle->ts) {

            topEle->rms++;
            log_trace("aprof_read: (ts[i]) %ld < (S[top].ts) %ld",
                      ts_map[i], topEle->ts);

            if (ts_map[i] != 0) {
                while (topEle->next != NULL) {
                    if (topEle->ts <= ts_map[i]) {
                        topEle->rms--;
                        break;
                    }

                    topEle = topEle->next;
                }
            }
        }

        ts_map[i] = count;
    }

}


void aprof_increment_rms() {
    struct stack_elem *topElement = top(shadow_stack);
    if (topElement != NULL) {
        topElement->rms++;
    }
}

void aprof_call_before(int funcId, unsigned long numCost) {
    count++;
    struct stack_elem *newEle = (struct stack_elem *) malloc(
            sizeof(struct stack_elem));
    newEle->funcId = funcId;
    newEle->ts = count;
    newEle->rms = 0;
    // newEle->cost update in aprof_return
    newEle->cost = numCost;
    push(shadow_stack, newEle);
    log_trace("aprof_call_before: push new element %d", funcId);

}


void aprof_return(unsigned long numCost) {

    struct stack_elem *topElement = top(shadow_stack);
    topElement->cost = numCost - topElement->cost;

    log_fatal(" ID %d ; RMS %ld ; Cost %ld ;",
              topElement->funcId,
              topElement->rms,
              topElement->cost
    );

    pop(shadow_stack);

    if (size(shadow_stack) >= 2) {
        struct stack_elem *secondEle = top(shadow_stack);
        secondEle->rms += topElement->rms;
        log_trace("aprof_return: top element rms is %d", secondEle->rms);
    }

}

#include <stack>
#include <map>

#include "aproflib.h"
#include "logger.h"

using namespace std;

unsigned long count = 0;
unsigned long bb_count = 0;
std::stack<ShadowStackElement *> shadow_stack;
std::map<unsigned long, unsigned long> ts_map;


void logger_init() {
    const char *FILENAME = "aprof_logger.txt";
    int LEVEL = 4;  // "TRACE" < "DEBUG" < "INFO" < "WARN" < "ERROR" < "FATAL"
    int QUIET = 1;
    FILE *fp = fopen(FILENAME, "w");
    log_set_level(LEVEL);
    log_set_fp(fp);
    log_set_quiet(QUIET);
}

int aprof_init() {
    logger_init();
}

void aprof_write(void *memory_addr, unsigned int length) {
    unsigned long start_addr = (unsigned long) memory_addr;
    unsigned long j = start_addr + length;

    for (unsigned long i = start_addr; i < j; i++) {
        ts_map[i] = count;
    }

    log_trace("aprof_write: memory_adrr is %ld, lenght is %ld, value is %ld",
              start_addr, length, count);

}

void aprof_read(void *memory_addr, unsigned int length) {
    unsigned long i;
    unsigned long start_addr = (unsigned long) memory_addr;
    ShadowStackElement *topEle = shadow_stack.top();

    for (i = start_addr; i < (start_addr + length); i++) {

        if (ts_map.find(i) != ts_map.end()) {

            // ts[w] > 0 and ts[w] < S[top]
            if (ts_map[i] < topEle->ts) {

                topEle->rms++;
                log_trace("aprof_read: (ts[i]) %ld < (S[top].ts) %ld",
                          ts_map[i], topEle->ts);

                std::stack<ShadowStackElement *> tempStack;
                while (!shadow_stack.empty()) {

                    topEle = shadow_stack.top();
                    if (topEle->ts <= ts_map[i]) {

                        topEle->rms--;
                        break;

                    } else {

                        tempStack.push(topEle);
                        shadow_stack.pop();
                    }
                }

                while (!tempStack.empty()) {
                    shadow_stack.push(tempStack.top());
                    tempStack.pop();
                }
            }

        } else {

            // w is not in map
            topEle->rms++;
        }

        ts_map[i] = count;
    }

}

void aprof_increment_cost() {
    bb_count++;

}

void aprof_increment_rms() {
    ShadowStackElement *topElement = shadow_stack.top();
    if (topElement) {
        topElement->rms++;
    }
}

void aprof_call_before(int funcId) {
    count++;
    ShadowStackElement *newEle = (ShadowStackElement *) malloc(
            sizeof(ShadowStackElement));
    newEle->funcId = funcId;
    newEle->ts = count;
    newEle->rms = 0;
    // newEle->cost update in aprof_call_after
    newEle->cost = bb_count;
    shadow_stack.push(newEle);
    log_trace("aprof_call_before: push new element %d", funcId);

}


void aprof_call_after() {

    ShadowStackElement *topElement = shadow_stack.top();
    topElement->cost = bb_count - topElement->cost;
    log_trace("aprof_call_after: top element cost is %ld", topElement->cost);

}

void collect() {
    if (!shadow_stack.empty()) {
        ShadowStackElement *topElement = shadow_stack.top();
        log_fatal(" ID %d ; RMS %ld ; Cost %ld ;",
                  topElement->funcId,
                  topElement->rms,
                  topElement->cost
        );
    }

}

void aprof_return() {

    aprof_call_after();
    collect();

    if (shadow_stack.size() >= 2) {
        ShadowStackElement *topEle = shadow_stack.top();
        shadow_stack.pop();
        ShadowStackElement *secondEle = shadow_stack.top();
        secondEle->rms += topEle->rms;
        log_trace("aprof_return: top element rms is %d", secondEle->rms);

    } else {
        ShadowStackElement *topEle = shadow_stack.top();
        shadow_stack.pop();
        log_trace("aprof_return: top element rms is %d", topEle->rms);
    }

}

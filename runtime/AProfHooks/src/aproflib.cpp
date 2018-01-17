#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/mman.h>
#include <vector>
#include <unordered_map>

#include "aproflib.h"
#include "logger.h"


using namespace std;


struct stack_elem {
    int funcId; // function id
    unsigned long ts; // time stamp
    unsigned long rms;
    unsigned long cost;
};

char log_str[BUFFERSIZE];
unsigned long count = 0;
struct stack_elem shadow_stack[200];
int stack_top = -1;


std::unordered_map<unsigned long, unsigned long> ts_map;


void logger_init() {
    const char *FILENAME = "aprof_logger.txt";
    int LEVEL = 4;  // "TRACE" < "DEBUG" < "INFO" < "WARN" < "ERROR" < "FATAL"
    int QUIET = 1;
    FILE *fp = fopen(FILENAME, "w");
    log_init(fp, LEVEL, QUIET);

}


int aprof_init() {
    // if we do not need debug, we should close this.
    logger_init();
}

char *_init_share_mem() {
    int fd = shm_open(APROF_MEM_LOG, O_RDWR | O_CREAT | O_EXCL, 0777);

    if (fd < 0) {
        fd = shm_open(APROF_MEM_LOG, O_RDWR, 0777);

    } else
        ftruncate(fd, BUFFERSIZE);

    char *pcBuffer = (char *) mmap(0, BUFFERSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    return pcBuffer;
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
    log_trace("aprof_read: top element funcID %d", shadow_stack[stack_top].funcId);
    log_trace("aprof_read: top element index %d", stack_top);

    for (i = start_addr; i < (start_addr + length); i++) {

        // We assume that w has been wrote before reading.
        // ts[w] > 0 and ts[w] < S[top]
        if (ts_map[i] < shadow_stack[stack_top].ts) {

            shadow_stack[stack_top].rms++;
            log_trace("aprof_read: (ts[i]) %ld < (S[top].ts) %ld",
                      ts_map[i], shadow_stack[stack_top].ts);

            if (ts_map[i] != 0) {
                for (int j = stack_top; j > 0; j--) {

                    if (shadow_stack[j].ts <= ts_map[i]) {
                        shadow_stack[j].rms--;
                        break;
                    }
                }
            }
        }

        ts_map[i] = count;
    }

}


void aprof_increment_rms() {
    shadow_stack[stack_top].rms++;

}

void aprof_call_before(int funcId, unsigned long numCost) {
    count++;
    stack_top++;
    shadow_stack[stack_top].funcId = funcId;
    shadow_stack[stack_top].ts = count;
    shadow_stack[stack_top].rms = 0;
    // newEle->cost update in aprof_return
    shadow_stack[stack_top].cost = numCost;
    log_trace("aprof_call_before: push new element %d", shadow_stack[stack_top].funcId);

}


void aprof_return(unsigned long numCost) {

    shadow_stack[stack_top].cost = numCost - shadow_stack[stack_top].cost;

    log_fatal(" ID %d ; RMS %ld ; Cost %ld ;",
              shadow_stack[stack_top].funcId,
              shadow_stack[stack_top].rms,
              shadow_stack[stack_top].cost
    );

//    char str[50];
//    sprintf(str, "ID %d , RMS %ld , Cost %ld \n",
//            shadow_stack[stack_top].funcId,
//            shadow_stack[stack_top].rms,
//            shadow_stack[stack_top].cost);
//    strcat(log_str, str);

    if (stack_top >= 1) {

        shadow_stack[stack_top - 1].rms += shadow_stack[stack_top].rms;
        stack_top--;
        log_trace("aprof_return: top element rms is %d", shadow_stack[stack_top].rms);

    } else {
        // log result to memory.
//        void *ptr = _init_share_mem();
//        strcpy((char *) ptr, log_str);
    }

}
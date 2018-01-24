#include "aproflib.h"
#include "logger.h"

// page table

// L0  28-31
// L1  19-27
// L2  10-18
// L3   0-9

void **pL0 = NULL;
void **pL1 = NULL;
void **pL2 = NULL;

unsigned long *pL3 = NULL;

unsigned long prev = 0;
unsigned long *prev_pL3 = NULL;

// page table

char log_str[BUFFERSIZE];
unsigned long count = 0;
struct stack_elem shadow_stack[200];
int stack_top = -1;


void init_page_table() {

    pL0 = (void **) malloc(sizeof(void *) * L0_TABLE_SIZE);
    memset(pL0, 0, sizeof(void *) * L0_TABLE_SIZE);
}

unsigned long query_page_table(unsigned long addr) {

    if ((addr & NEG_L3_MASK) == prev) {
        return prev_pL3[addr & L3_MASK];
    }


    unsigned long tmp = (addr & L0_MASK) >> 28;

    if (pL0[tmp] == NULL) {
        return 0;
    }

    pL1 = pL0[tmp];

    tmp = (addr & L1_MASK) >> 19;

    if (pL1[tmp] == NULL) {
        return 0;
    }

    pL2 = pL1[tmp];

    tmp = (addr & L2_MASK) >> 10;

    if (pL2[tmp] == NULL) {
        return 0;
    }

    pL3 = (unsigned long *) pL2[tmp];

    prev = addr & NEG_L3_MASK;
    prev_pL3 = pL3;

    return pL3[addr & L3_MASK];
}


void insert_page_table(unsigned long addr, unsigned long count) {

    if ((addr & NEG_L3_MASK) == prev) {
        prev_pL3[addr & L3_MASK] = count;
        return;
    }


    unsigned long tmp = (addr & L0_MASK) >> 28;

    if (pL0[tmp] == NULL) {
        pL0[tmp] = (void **) malloc(sizeof(void *) * L1_TABLE_SIZE);
        memset(pL0[tmp], 0, sizeof(void *) * L1_TABLE_SIZE);
    }

    pL1 = pL0[tmp];

    tmp = (addr & L1_MASK) >> 19;

    if (pL1[tmp] == NULL) {
        pL1[tmp] = (void **) malloc(sizeof(void *) * L1_TABLE_SIZE);
        memset(pL1[tmp], 0, sizeof(void *) * L1_TABLE_SIZE);
    }

    pL2 = pL1[tmp];

    tmp = (addr & L2_MASK) >> 10;

    if (pL2[tmp] == NULL) {
        pL2[tmp] = (unsigned long *) malloc(sizeof(unsigned long) * L3_TABLE_SIZE);
        memset(pL2[tmp], 0, sizeof(unsigned long) * L3_TABLE_SIZE);
    }

    pL3 = (unsigned long *) pL2[tmp];

    prev = addr & NEG_L3_MASK;
    prev_pL3 = pL3;

    pL3[addr & L3_MASK] = count;

}

void destroy_page_table() {
    int i, j, k;

    for (i = 0; i < L0_TABLE_SIZE; i++) {
        if (pL0[i] == NULL) {
            continue;
        }

        pL1 = (void **) pL0[i];

        for (j = 0; j < L1_TABLE_SIZE; j++) {
            if (pL1[j] == NULL) {
                continue;
            }

            pL2 = (void **) pL1[j];

            for (k = 0; k < L1_TABLE_SIZE; k++) {
                if (pL2[k] != NULL) {
                    free(pL2[k]);
                }
            }

            free(pL2);
        }

        free(pL1);
    }

    free(pL0);
}

void logger_init() {
    const char *FILENAME = "aprof_logger.txt";
    int LEVEL = 4;  // "TRACE" < "DEBUG" < "INFO" < "WARN" < "ERROR" < "FATAL"
    int QUIET = 1;
    FILE *fp = fopen(FILENAME, "w");
    log_init(fp, LEVEL, QUIET);

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

int aprof_init() {
    // if we do not need debug, we should close this.
//    logger_init();
//    _init_share_mem();
    init_page_table();
}


void aprof_write(void *memory_addr, unsigned int length) {
    unsigned long start_addr = (unsigned long) memory_addr;

    for (unsigned long i = start_addr; i < start_addr + length; i++) {
        insert_page_table(i, count);

    }

//    log_trace("aprof_write: memory_adrr is %ld, lenght is %ld, value is %ld",
//              start_addr, length, count);

}

void aprof_read(void *memory_addr, unsigned int length) {
    unsigned long i;
    unsigned long start_addr = (unsigned long) memory_addr;

//    log_trace("aprof_read: top element funcID %d", shadow_stack[stack_top].funcId);
//    log_trace("aprof_read: top element index %d", stack_top);

    for (i = start_addr; i < (start_addr + length); i++) {

        // We assume that w has been wrote before reading.
        // ts[w] > 0 and ts[w] < S[top]
        unsigned long ts_w = query_page_table(i);
        if (ts_w < shadow_stack[stack_top].ts) {

            shadow_stack[stack_top].rms++;
//            log_trace("aprof_read: (ts[i]) %ld < (S[top].ts) %ld",
//                      ts_w, shadow_stack[stack_top].ts);

            if (ts_w != 0) {
                for (int j = stack_top; j > 0; j--) {

                    if (shadow_stack[j].ts <= ts_w) {
                        shadow_stack[j].rms--;
                        break;
                    }
                }
            }
        }

        insert_page_table(i, count);
    }

}


void aprof_increment_rms() {
    shadow_stack[stack_top].rms++;

}

//void aprof_call_before(int funcId, unsigned long numCost)
void aprof_call_before(int funcId) {
    count++;
    stack_top++;
    shadow_stack[stack_top].funcId = funcId;
    shadow_stack[stack_top].ts = count;
    shadow_stack[stack_top].rms = 0;
    // newEle->cost update in aprof_return
    shadow_stack[stack_top].cost = 0;
//    log_trace("aprof_call_before: push new element %d", shadow_stack[stack_top].funcId);

}


void aprof_return(unsigned long numCost, unsigned long rms) {

    shadow_stack[stack_top].cost += numCost;
    shadow_stack[stack_top].rms += rms;

//    log_fatal(" ID %d ; RMS %ld ; Cost %ld ;",
//              shadow_stack[stack_top].funcId,
//              shadow_stack[stack_top].rms,
//              shadow_stack[stack_top].cost
//    );

    char str[50];
    sprintf(str, "ID %d , RMS %ld , Cost %ld \n",
            shadow_stack[stack_top].funcId,
            shadow_stack[stack_top].rms,
            shadow_stack[stack_top].cost);
    strcat(log_str, str);

    if (stack_top >= 1) {

        shadow_stack[stack_top - 1].rms += shadow_stack[stack_top].rms;
        shadow_stack[stack_top - 1].cost += shadow_stack[stack_top].cost;
        stack_top--;
//        log_trace("aprof_return: top element rms is %d", shadow_stack[stack_top].rms);

    } else {
        // log result to memory.
        void *ptr = _init_share_mem();
        strcpy((char *) ptr, log_str);
        destroy_page_table();
    }

}

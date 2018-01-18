#include "aproflib.h"
#include "logger.h"

// page table

// L0  46-47
// L1  37-45
// L2  28-36
// L3  19-27
// L4  10-18
// L5   0-9

void ** pL0 = NULL;
void ** pL1 = NULL;
void ** pL2 = NULL;
void ** pL3 = NULL;
void ** pL4 = NULL;

unsigned long * pL5 = NULL;

// page table

char log_str[BUFFERSIZE];
unsigned long count = 0;
struct stack_elem shadow_stack[200];
int stack_top = -1;


void init_page_table()
{
    pL0 = (void **)malloc(sizeof(void *) * L0_TABLE_SIZE);
    memset(pL0, 0, sizeof(void *) * L0_TABLE_SIZE);
}

unsigned long query_page_table(unsigned long addr)
{
    unsigned long tmp = (addr&L0_MASK) >> 46;

    if(pL0[tmp] == NULL)
    {
        return 0;
    }

    pL1 = pL0[tmp];

    tmp = (addr & L1_MASK) >> 37;

    if(pL1[tmp] == NULL)
    {
        return 0;
    }

    pL2 = pL1[tmp];

    tmp = (addr & L2_MASK) >> 28;

    if(pL2[tmp] == NULL)
    {
        return 0;
    }

    pL3 = pL2[tmp];

    tmp = (addr & L3_MASK) >> 19;

    if(pL3[tmp] == NULL)
    {
        return 0;
    }

    pL4 = pL3[tmp];

    tmp = (addr & L4_MASK) >> 10;

    if(pL4[tmp] == NULL)
    {
        return 0;
    }

    pL5 = (unsigned long *)pL4[tmp];

    return pL5[addr&L5_MASK];
}


void insert_page_table(unsigned long addr, unsigned long count)
{
    unsigned long tmp = (addr&L0_MASK) >> 46;

    if(pL0[tmp] == NULL)
    {
        pL0[tmp] = (void **)malloc(sizeof(void *) * L1_TABLE_SIZE);
        memset(pL0[tmp], 0, sizeof(void *) * L1_TABLE_SIZE);
    }

    pL1 = pL0[tmp];

    tmp = (addr & L1_MASK) >> 37;

    if(pL1[tmp] == NULL)
    {
        pL1[tmp] = (void **)malloc(sizeof(void *) * L1_TABLE_SIZE);
        memset(pL1[tmp], 0, sizeof(void *) * L1_TABLE_SIZE);
    }

    pL2 = pL1[tmp];

    tmp = (addr & L2_MASK) >> 28;

    if(pL2[tmp] == NULL)
    {
        pL2[tmp] = (void **)malloc(sizeof(void *) * L1_TABLE_SIZE);
        memset(pL2[tmp], 0, sizeof(void *) * L1_TABLE_SIZE);
    }

    pL3 = pL2[tmp];

    tmp = (addr & L3_MASK) >> 19;

    if(pL3[tmp] == NULL)
    {
        pL3[tmp] = (void **)malloc(sizeof(void *) * L1_TABLE_SIZE);
        memset(pL3[tmp], 0, sizeof(void *) * L1_TABLE_SIZE);
    }

    pL4 = pL3[tmp];

    tmp = (addr & L4_MASK) >> 10;

    if(pL4[tmp] == NULL)
    {
        pL4[tmp] = (unsigned long *)malloc(sizeof(unsigned long) * L4_TABLE_SIZE);
        memset(pL4[tmp], 0, sizeof(void *) * L1_TABLE_SIZE);
    }

    pL5 = (unsigned long *)pL4[tmp];

    pL5[addr&L5_MASK] = count;

}

void destroy_page_table()
{
    int i, j, k, l, m;

    for(i = 0; i < L0_TABLE_SIZE; i ++)
    {
        if(pL0[i] == NULL)
        {
            continue;
        }

        pL1 = (void **)pL0[i];

        for(j = 0; j < L1_TABLE_SIZE; j ++ )
        {
            if(pL1[j] == NULL)
            {
                continue;
            }

            pL2 = (void **)pL1[j];

            for(k = 0; k < L1_TABLE_SIZE; k ++ )
            {
                if(pL2[k] == NULL)
                {
                    continue;
                }

                pL3 = (void **)pL2[k];

                for(l = 0; l < L1_TABLE_SIZE; l ++)
                {
                    if(pL3[l] == NULL)
                    {
                        continue;
                    }

                    pL4 = (void **)pL3[l];

                    for(m = 0; m < L1_TABLE_SIZE; m ++ )
                    {
                        if(pL4[m] != NULL)
                        {
                            free(pL4[m]);
                        }
                    }

                    free(pL4);
                }

                free(pL3);
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
    logger_init();
//    _init_share_mem();
    init_page_table();
}


void aprof_write(void *memory_addr, unsigned int length) {
    unsigned long start_addr = (unsigned long) memory_addr;

    for (unsigned long i = start_addr; i < start_addr + length; i++) {
        insert_page_table(i, count);

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
        unsigned long ts_w = query_page_table(i);
        if (ts_w < shadow_stack[stack_top].ts) {

            shadow_stack[stack_top].rms++;
            log_trace("aprof_read: (ts[i]) %ld < (S[top].ts) %ld",
                      ts_w, shadow_stack[stack_top].ts);

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
        destroy_page_table();
    }

}
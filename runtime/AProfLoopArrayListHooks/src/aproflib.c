#include "aproflib.h"

// page table
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


struct log_address store_stack[1];

// share memory
int fd;
void *pcBuffer = NULL;
unsigned int store_size = sizeof(struct log_address);

unsigned long cache_addr = 0;
int last_return = 1;

// sampling
static int old_value = -1;

// aprof api

void aprof_init() {

    // init share memory
    fd = shm_open(APROF_MEM_LOG, O_RDWR | O_CREAT | O_EXCL, 0777);

    if (fd < 0) {
        fd = shm_open(APROF_MEM_LOG, O_RDWR, 0777);

    } else
        ftruncate(fd, BUFFERSIZE);

    pcBuffer = (void *) mmap(0, BUFFERSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

}

unsigned long aprof_query_page_table(unsigned long addr) {

    if ((addr & NEG_L3_MASK) == prev) {
        return prev_pL3[addr & L3_MASK];
    }

    unsigned long tmp = (addr & L0_MASK) >> 28;

    if (pL0[tmp] == NULL) {
        return 0;
    }

    pL1 = (void **) pL0[tmp];

    tmp = (addr & L1_MASK) >> 19;

    if (pL1[tmp] == NULL) {
        return 0;
    }

    pL2 = (void **) pL1[tmp];

    tmp = (addr & L2_MASK) >> 10;

    if (pL2[tmp] == NULL) {
        return 0;
    }

    pL3 = (unsigned long *) pL2[tmp];

    prev = addr & NEG_L3_MASK;
    prev_pL3 = pL3;

    return pL3[addr & L3_MASK];
}


void aprof_insert_page_table(unsigned long addr, unsigned long count) {

    if ((addr & NEG_L3_MASK) == prev) {
        prev_pL3[addr & L3_MASK] = count;
        return;
    }


    unsigned long tmp = (addr & L0_MASK) >> 28;

    if (pL0[tmp] == NULL) {
        pL0[tmp] = (void **) malloc(sizeof(void *) * L1_TABLE_SIZE);
        memset(pL0[tmp], 0, sizeof(void *) * L1_TABLE_SIZE);
    }

    pL1 = (void **) pL0[tmp];

    tmp = (addr & L1_MASK) >> 19;

    if (pL1[tmp] == NULL) {

        pL1[tmp] = (void **) malloc(sizeof(void *) * L1_TABLE_SIZE);
        memset(pL1[tmp], 0, sizeof(void *) * L1_TABLE_SIZE);
    }

    pL2 = (void **) pL1[tmp];

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

void aprof_dump(void *memory_addr, int length) {
    unsigned long addr = (unsigned long) memory_addr;
//    if (cache_addr != addr) {
        store_stack[0].numCost = addr;
        store_stack[0].length = length;
        memcpy(pcBuffer, &(store_stack), store_size);
        pcBuffer += store_size;
        last_return = 0;
//        cache_addr = addr;
//    }
}

void aprof_return(unsigned long numCost) {
    if (last_return != 1) {
        store_stack[0].numCost = numCost;
        store_stack[0].length = 0;
        memcpy(pcBuffer, &(store_stack), store_size);
        pcBuffer += store_size;
        last_return = 1;
    }

}

static double aprof_rand_val(int seed) {
    const long a = 16807;  // Multiplier
    const long m = 2147483647;  // Modulus
    const long q = 127773;  // m div a
    const long r = -2836;  // m mod a
    static long x;               // Random int value
    long x_div_q;         // x divided by q
    long x_mod_q;         // x modulo q
    long x_new;           // New x value

    // Set the seed if argument is non-zero and then return zero
    if (seed > 0) {
        x = seed;
        return (0.0);
    }

    // RNG using integer arithmetic
    x_div_q = x / q;
    x_mod_q = x % q;
    x_new = (a * x_mod_q) - (r * x_div_q);
    if (x_new > 0)
        x = x_new;
    else
        x = x_new + m;

    // Return a random value between 0.0 and 1.0
    return ((double) x / m);
}


int aprof_geo(int iRate) {
    double p = 1 / (double) iRate;
    double z;                     // Uniform random number (0 < z < 1)
    double geo_value;             // Computed geometric value to be returned

    do {
        // Pull a uniform random number (0 < z < 1)
        do {
            z = aprof_rand_val(0);
        } while ((z == 0) || (z == 1));

        // Compute geometric random variable using inversion method
        geo_value = (int) (log(z) / log(1.0 - p)) + 1;
    } while ((int) geo_value == old_value + 1);

    old_value = (int) geo_value;
    // log sampling call chain number
    return old_value;
}

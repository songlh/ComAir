#include "aproflib.h"

// page table

struct log_address store_stack[1];

// share memory
int fd;
void *pcBuffer = NULL;
unsigned int store_size = sizeof(struct log_address);

unsigned long cache_addr = 0;
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

void aprof_dump(void *memory_addr, int length) {
    unsigned long addr = (unsigned long) memory_addr;
    if (cache_addr != addr) {
        store_stack[0].numCost = (unsigned long) memory_addr;
        store_stack[0].length = length;
        memcpy(pcBuffer, &(store_stack), store_size);
        pcBuffer += store_size;
        cache_addr = addr;
    }
}

void aprof_return(unsigned long numCost) {
    store_stack[0].numCost = numCost;
    store_stack[0].length = 0;
    memcpy(pcBuffer, &(store_stack), store_size);
    pcBuffer += store_size;
}

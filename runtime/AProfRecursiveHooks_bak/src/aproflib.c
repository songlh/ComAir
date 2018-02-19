#include "aproflib.h"

// share memory
int fd;
void *pcBuffer;
unsigned int store_size = sizeof(struct stack_elem);
struct stack_elem store_stack[1];
unsigned long shadow_stack[STACK_SIZE];
int stack_top = -1;

void aprof_init() {
    // init share memory
    fd = shm_open(APROF_MEM_LOG, O_RDWR | O_CREAT | O_EXCL, 0777);

    if (fd < 0) {
        fd = shm_open(APROF_MEM_LOG, O_RDWR, 0777);

    } else
        ftruncate(fd, BUFFERSIZE);

    pcBuffer = (void *) mmap(0, BUFFERSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

}

void aprof_call_in(int funcID, unsigned long numCost) {

    stack_top++;
    shadow_stack[stack_top] = numCost;

}

void aprof_return(int funcID, unsigned long numCost) {

    store_stack[0].cost = numCost - shadow_stack[stack_top];
    store_stack[0].stack_index = stack_top;
    memcpy(pcBuffer, &(store_stack), store_size);
    pcBuffer += store_size;
    stack_top--;

}

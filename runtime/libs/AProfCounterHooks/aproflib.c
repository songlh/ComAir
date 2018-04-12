#include "aproflib.h"


int recursiveFunc[STACK_SIZE] = {0};
int LoopFunc[STACK_SIZE]={0};

int sizeOffset = sizeof(int) * STACK_SIZE;

// share memory
int fd;
void *pcBuffer;


// aprof api
void aprof_init() {

    // init share memory
    fd = shm_open(APROF_MEM_LOG, O_RDWR | O_CREAT | O_EXCL, 0777);

    if (fd < 0) {
        fd = shm_open(APROF_MEM_LOG, O_RDWR, 0777);

    } else
        ftruncate(fd, BUFFERSIZE);

    pcBuffer = (void *) mmap(0, BUFFERSIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, fd, 0);

}

void updater(int id, int flag) {
    if (flag == 0) {
        recursiveFunc[id]++;
    } else {
        LoopFunc[id]++;
    }
}


void dump() {
    memcpy(pcBuffer, &(recursiveFunc), sizeOffset);
    pcBuffer += sizeOffset;
    memcpy(pcBuffer, &(LoopFunc), sizeOffset);
}

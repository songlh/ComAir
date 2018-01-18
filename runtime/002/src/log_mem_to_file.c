#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/mman.h>


#define BUFFERSIZE (unsigned long)1 << 33

#define APROF_MEM_LOG "/aprof_log.log"


int main(int argc, char *argv[]) {
    int fd = shm_open(APROF_MEM_LOG, O_RDWR | O_CREAT | O_EXCL, 0777);

    if (fd < 0) {
        fd = shm_open(APROF_MEM_LOG, O_RDWR, 0777);

    } else
        ftruncate(fd, BUFFERSIZE);

    void *ptr = mmap(NULL, BUFFERSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    puts("start reading data....");
    puts((char *) ptr);
    puts("read over");
    shm_unlink(APROF_MEM_LOG);
    close(fd);
    return 0;
}

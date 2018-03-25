#ifndef APROFHOOKS_LIBRARY_H
#define APROFHOOKS_LIBRARY_H

#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>


/* ---- page table ---- */
#define STACK_SIZE 2000
/*---- end ----*/

/*---- share memory ---- */
#define BUFFERSIZE (unsigned long) 1UL << 34
#define APROF_MEM_LOG "/aprof_loop_arry_list_log.log"

/*---- end ----*/

/*---- run time lib api ----*/

struct log_address {
    unsigned long  numCost;
    int  length;
};

void aprof_init();

void aprof_dump(void *memory_addr, int length);

void aprof_return(unsigned long numCost);

/*---- end ----*/

#endif

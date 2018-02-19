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


struct stack_elem {
//    int funcId; // function id
    int stack_index;
    unsigned long cost;

};

#define STACK_SIZE 2000

/*---- share memory ---- */
#define BUFFERSIZE (unsigned long) 1UL << 34
#define APROF_MEM_LOG "/aprof_log.log"

char * aprof_init_share_mem();

/*---- end ----*/


void aprof_init();

void aprof_call_in(int funcID, unsigned long numCost);

void aprof_return(int funcID, unsigned long numCost);

#endif

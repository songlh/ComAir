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


// array size
#define STACK_SIZE 20000

/*---- share memory ---- */
#define BUFFERSIZE (unsigned long) 1UL << 34
#define APROF_MEM_LOG "/aprof_counter_log.log"


void update(int, int);
void dump();

#endif

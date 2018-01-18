#ifndef APROFHOOKS_LIBRARY_H
#define APROFHOOKS_LIBRARY_H

#include <fcntl.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>


// page table

#define PAGE_SIZE     4096
#define L0_TABLE_SIZE 4
#define L1_TABLE_SIZE 512
#define L4_TABLE_SIZE 1024

#define L0_MASK  0xC00000000000
#define L1_MASK  0x3FE000000000
#define L2_MASK  0x1FF0000000
#define L3_MASK  0xFF80000
#define L4_MASK  0x7FC00
#define L5_MASK  0x3FF

void init_page_table();

unsigned long query_page_table(unsigned long address);

void insert_page_table(unsigned long address, unsigned long count);

void destroy_page_table();

// page table


#define BUFFERSIZE (unsigned long)1 << 24
#define APROF_MEM_LOG "/aprof_log.log"

struct stack_elem {
    int funcId; // function id
    unsigned long ts; // time stamp
    unsigned long rms;
    unsigned long cost;
};

void logger_init();

char * _init_share_mem();

int aprof_init();

void aprof_write(void *memory_addr, unsigned int length);

void aprof_read(void *memory, unsigned int length);

void aprof_increment_cost();

void aprof_increment_rms();

void aprof_call_before(int funcId, unsigned long numCost);

void aprof_collect();

void aprof_call_after();

void aprof_return(unsigned long numCost);


#endif

#ifndef COMAIR_APROFHOOK_H
#define COMAIR_APROFHOOK_H

#include "common.h"
#include "hashMap.h"
#include "inputPro.h"
#include "logger.h"


char *FILENAME = "aprof_logger.txt";
int LEVEL = 0;  // "TRACE" < "DEBUG" < "INFO" < "WARN" < "ERROR" < "FATAL"
int QUIET = 1;

void logger_init();

int aprof_init();

void aprof_write(void *memory_addr, unsigned int length);

void aprof_read(void *memory, unsigned int length);

void aprof_increment_cost();

void aprof_call_before(char *FuncName);

void aprof_call_after();

void aprof_return();

#endif //COMAIR_APROFHOOK_H

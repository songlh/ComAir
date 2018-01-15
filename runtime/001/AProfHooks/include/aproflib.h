#ifndef APROFHOOKS_LIBRARY_H
#define APROFHOOKS_LIBRARY_H


typedef struct _ShadowStackElem {
    int  funcId; // function id
    unsigned long ts; // time stamp
    unsigned long rms;
    unsigned long cost;
} ShadowStackElement;


#ifdef __cplusplus
extern "C" {

void logger_init();

int aprof_init();

void aprof_write(void *memory_addr, unsigned int length);

void aprof_read(void *memory, unsigned int length);

void aprof_increment_cost();

void aprof_increment_rms();

void aprof_call_before(int funcId);

void aprof_collect();

void aprof_call_after();

void aprof_return();

};
#endif

#endif

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
    int funcId; // function id
    unsigned long long cost;
    char in_out; // i / o
};

/*---- share memory ---- */
#define BUFFERSIZE (unsigned long) 1 << 33
#define APROF_MEM_LOG "/aprof_log.log"

char * aprof_init_share_mem();

/*---- end ----*/


void aprof_init();

void aprof_call_in(int funcID, unsigned long numCost);

void aprof_return(int funcID, unsigned long numCost);

/*---- sampling generator ----*/

//===========================================================================
//=  Function to generate geometrically distributed random variables        =
//=    - Input:  Probability of success p                                   =
//=    - Output: Returns with geometrically distributed random variable     =
//===========================================================================

int aprof_geo(int iRate);            // Returns a geometric random variable

//=========================================================================
//= Multiplicative LCG for generating uniform(0.0, 1.0) random numbers    =
//=   - x_n = 7^5*x_(n-1)mod(2^31 - 1)                                    =
//=   - With x seeded to 1 the 10000th x value should be 1043618065       =
//=   - From R. Jain, "The Art of Computer Systems Performance Analysis," =
//=     John Wiley & Sons, 1991. (Page 443, Figure 26.2)                  =
//=========================================================================
static double aprof_rand_val(int seed);    // Jain's RNG

/*---- end ----*/

/* ---- logger to file ---- */

#define LOG_VERSION "0.1.0"

typedef void (*log_LockFn)(void *udata, int lock);

enum {
    LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL
};


#define log_trace(...) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)  log_log(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)  log_log(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)


void log_set_udata(void *udata);

void log_set_lock(log_LockFn fn);

void log_set_fp(FILE *fp);

void log_set_level(int level);

void log_set_quiet(int enable);

void log_init(FILE *fp, int level, int enable);

void log_log(int level, const char *file, int line, const char *fmt, ...);

static struct {
    void *udata;
    log_LockFn lock;
    FILE *fp;
    int level;
    int quiet;
} L;


static const char *level_names[] = {
        "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

/* ---- end ---- */


#endif

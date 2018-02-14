#include "aproflib.h"

// share memory
int fd;
void *pcBuffer;
unsigned int struct_size = sizeof(struct stack_elem);
struct stack_elem shadow_stack[1];

// sampling
unsigned long sampling_count = 0;
static int old_value = -1;

// logger

static void lock(void) {
    if (L.lock) {
        L.lock(L.udata, 1);
    }
}

static void unlock(void) {
    if (L.lock) {
        L.lock(L.udata, 0);
    }
}

void log_set_udata(void *udata) {
    L.udata = udata;
}

void log_set_lock(log_LockFn fn) {
    L.lock = fn;
}

void log_set_fp(FILE *fp) {
    L.fp = fp;
}

void log_set_level(int level) {
    L.level = level;
}

void log_set_quiet(int enable) {
    L.quiet = enable ? 1 : 0;
}

void log_init(FILE *fp, int level, int enable) {
    L.fp = fp;
    L.level = level;
    L.quiet = enable ? 1 : 0;
}

void log_log(int level, const char *file, int line, const char *fmt, ...) {
    if (level < L.level) {
        return;
    }

    /* Acquire lock */
    lock();

    /* Log to file */
    if (L.fp) {
        va_list args;
        va_start(args, fmt);
        vfprintf(L.fp, fmt, args);
        va_end(args);
        fprintf(L.fp, "\n");
    }

    /* Release lock */
    unlock();
}

void aprof_logger_init() {
    const char *FILENAME = "aprof_logger.txt";
    int LEVEL = 4;  // "TRACE" < "DEBUG" < "INFO" < "WARN" < "ERROR" < "FATAL"
    int QUIET = 1;
    FILE *fp = fopen(FILENAME, "w");
    log_init(fp, LEVEL, QUIET);

}


void aprof_init() {

    // init share memory
    fd = shm_open(APROF_MEM_LOG, O_RDWR | O_CREAT | O_EXCL, 0777);

    if (fd < 0) {
        fd = shm_open(APROF_MEM_LOG, O_RDWR, 0777);

    } else
        ftruncate(fd, BUFFERSIZE);

     pcBuffer = (void *) mmap(0, BUFFERSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

//    aprof_logger_init();

}


void aprof_return(int funcID, unsigned long numCost) {

    shadow_stack[0].funcId = funcID;
    shadow_stack[0].cost = numCost;
    memcpy(pcBuffer, &(shadow_stack[0]), struct_size);
    pcBuffer += struct_size;
//    "ID %d ; Cost %ld ;"
//    log_fatal("ID %d ; Cost %ld ;",
//              funcID,
//              numCost
//    );
}


static double aprof_rand_val(int seed) {
    const long a = 16807;  // Multiplier
    const long m = 2147483647;  // Modulus
    const long q = 127773;  // m div a
    const long r = -2836;  // m mod a
    static long x;               // Random int value
    long x_div_q;         // x divided by q
    long x_mod_q;         // x modulo q
    long x_new;           // New x value

    // Set the seed if argument is non-zero and then return zero
    if (seed > 0) {
        x = seed;
        return (0.0);
    }

    // RNG using integer arithmetic
    x_div_q = x / q;
    x_mod_q = x % q;
    x_new = (a * x_mod_q) - (r * x_div_q);
    if (x_new > 0)
        x = x_new;
    else
        x = x_new + m;

    // Return a random value between 0.0 and 1.0
    return ((double) x / m);
}


int aprof_geo(int iRate) {
    double p = 1 / (double) iRate;
    double z;                     // Uniform random number (0 < z < 1)
    double geo_value;             // Computed geometric value to be returned

    do {
        // Pull a uniform random number (0 < z < 1)
        do {
            z = aprof_rand_val(0);
        } while ((z == 0) || (z == 1));

        // Compute geometric random variable using inversion method
        geo_value = (int) (log(z) / log(1.0 - p)) + 1;
    } while ((int) geo_value == old_value + 1);

    old_value = (int) geo_value;
    // log sampling call chain number
//    sampling_count = count - sampling_count;
//    log_fatal("sampling count: %ld;", sampling_count);
    return old_value;
}

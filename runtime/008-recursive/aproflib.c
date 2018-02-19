#include "aproflib.h"

// share memory
int fd;
void *pcBuffer;
unsigned int store_size = sizeof(struct stack_elem);
struct stack_elem store_stack[1];
unsigned long shadow_stack[STACK_SIZE];
int stack_top = -1;

// sampling
int sampling = 0;
static int old_value = -1;

void aprof_init() {
    // init share memory
    fd = shm_open(APROF_MEM_LOG, O_RDWR | O_CREAT | O_EXCL, 0777);

    if (fd < 0) {
        fd = shm_open(APROF_MEM_LOG, O_RDWR, 0777);

    } else
        ftruncate(fd, BUFFERSIZE);

    pcBuffer = (void *) mmap(0, BUFFERSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

}

void aprof_call_in(int funcID, unsigned long numCost) {

    stack_top++;
    shadow_stack[stack_top] = numCost;

}

void aprof_return(int funcID, unsigned long numCost) {

    store_stack[0].cost = numCost - shadow_stack[stack_top];
    store_stack[0].stack_index = stack_top;

    if (sampling <= 0) {
        memcpy(pcBuffer, &(store_stack), store_size);
        pcBuffer += store_size;
        sampling = aprof_geo(SAMPLING_RATE);
    }

    stack_top--;
    sampling--;

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
    return old_value;

}

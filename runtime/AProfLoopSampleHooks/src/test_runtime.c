#include <stdio.h>
#include "aproflib.h"


int main() {

    aprof_init();
    aprof_call_before(1);
    aprof_call_before(2);

    aprof_increment_rms(10);
    aprof_increment_rms(5);

    aprof_return(3);
    aprof_return(2);
    return 0;
}

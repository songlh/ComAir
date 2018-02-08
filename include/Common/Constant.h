#ifndef COMAIR_CONSTANT_H
#define COMAIR_CONSTANT_H

#define APROF_INSERT_FLAG "aprof_insert_flag"

enum {
    INIT = 0,
    WRITE = 1,
    READ = 2,
    CALL_BEFORE = 3,
    RETURN = 4,
    INCREMENT_RMS = 5,
    COST_UPDATE = 6,
};

#define BB_COST_FLAG "bb_cost_flag"
#define IGNORE_FUNC_FLAG "ignore_func_flag"

#define CLONE_FUNCTION_PREFIX "$aprof$"

#endif //COMAIR_CONSTANT_H

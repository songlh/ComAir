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
#define IGNORE_OPTIMIZED_FLAG "ignore_optimized_flag"

#define CLONE_FUNCTION_PREFIX "$aprof$"

#define ARRAY_LIST_INSERT "aprof_array_list_hook"

#endif //COMAIR_CONSTANT_H

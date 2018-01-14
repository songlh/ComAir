#ifndef _HEAD_H
#define _HEAD_H

#include <stdlib.h>
#include "brg_types.h"

#define bots_message(msg, ...) \
{\
  fprintf(stdout, msg , ##__VA_ARGS__);\
}


#define bots_debug(msg, ...) \
{\
  fprintf(stdout, msg , ##__VA_ARGS__);\
}

/**********************************/
/* random number generator state  */
/**********************************/
struct state_t {
  u_int8_t state[20];
};

struct node_t {
  int height;        // depth of this node in the tree
  int numChildren;   // number of children, -1 => not yet determined

  /* for RNG state associated with this node */
  struct state_t state;
};

typedef struct node_t Node;

#endif

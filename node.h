#ifndef __NODE__
#define __NODE__

#include "utils.h"



/**
 * Data structure containing the node information
 */
struct node_s{
    int priority;
    float acc_reward;
    int depth;
    int num_childs;
    move_t move;
    // Added initial move so we don't have to backprogate
    move_t initial_move;
    state_t state;
    struct node_s* parent;
};

typedef struct node_s node_t;


#endif

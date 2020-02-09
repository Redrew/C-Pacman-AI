#ifndef __AI__
#define __AI__

#include <stdint.h>
#include <unistd.h>
#include "node.h"
#include "priority_queue.h"

#define Y 0.99

void initialize_ai();

move_t get_next_move( state_t init_state, int budget, propagation_t propagation, char* stats );

extern int total_generated;
extern int total_expanded;
extern int global_max_depth;
extern float total_time;

#endif

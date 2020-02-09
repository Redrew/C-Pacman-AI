#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <assert.h>

#include "ai.h"
#include "utils.h"
#include "priority_queue.h"

#define SET_MAX(a, b) if (a < b) a = b

#define DISCOUNT 0.99
#define LIFE_LOSS_PENALTY 10
#define INVICIBILITY_REWARD 10
#define GAME_OVER_PENALTY 100

struct heap h;
move_t prev_a = 0;
int total_generated = 0;
int total_expanded = 0;
int global_max_depth = 0;
float total_time = 0;

float get_reward( node_t* n );

/**
 * Function called by pacman.c
*/
void initialize_ai(){
	heap_init(&h);
}

/**
 * function to copy a src into a dst state
*/
void copy_state(state_t* dst, state_t* src){
	//Location of Ghosts and Pacman
	memcpy( dst->Loc, src->Loc, 5*2*sizeof(int) );

    //Direction of Ghosts and Pacman
	memcpy( dst->Dir, src->Dir, 5*2*sizeof(int) );

    //Default location in case Pacman/Ghosts die
	memcpy( dst->StartingPoints, src->StartingPoints, 5*2*sizeof(int) );

    //Check for invincibility
    dst->Invincible = src->Invincible;
    
    //Number of pellets left in level
    dst->Food = src->Food;
    
    //Main level array
	memcpy( dst->Level, src->Level, 29*28*sizeof(int) );

    //What level number are we on?
    dst->LevelNumber = src->LevelNumber;
    
    //Keep track of how many points to give for eating ghosts
    dst->GhostsInARow = src->GhostsInARow;

    //How long left for invincibility
    dst->tleft = src->tleft;

    //Initial points
    dst->Points = src->Points;

    //Remaining Lives
    dst->Lives = src->Lives;   

}

node_t* create_init_node( state_t* init_state ){
	node_t *new_n = (node_t *) malloc(sizeof(node_t));
	assert(new_n);
	new_n->parent = NULL;	
	new_n->priority = 0;
	new_n->depth = 0;
	new_n->num_childs = 0;
	new_n->move = prev_a;
	copy_state(&(new_n->state), init_state);
	new_n->acc_reward = 0;
	return new_n;
}

float heuristic( node_t* n ){
	float h = 0;
	
	// h(n) = i − l − g - d, i: invicible, l: lost life, g: game over,
	// d: direction change
	if (n->parent->state.Lives > n->state.Lives && n->depth < 15) h -= LIFE_LOSS_PENALTY;
	if (!n->parent->state.Invincible && n->state.Invincible) 
		h += INVICIBILITY_REWARD;
	if (n->state.Lives == -1) h -= GAME_OVER_PENALTY;

	return h;
}

float get_reward ( node_t* n ){
	float reward = 0;
	
	if (n->parent) 
		reward = heuristic(n) + n->state.Points - n->parent->state.Points;
	float discount = pow(DISCOUNT,n->depth);
	
	return discount * reward;
}

/**
 * Apply an action to node n and return a new node resulting from executing the action
*/
bool apply_action(node_t* n, node_t* new_node, move_t action ){
	bool changed_dir = false;

    changed_dir = execute_move_t( &(new_node->state), action );	
	// Keep track of the first move in this subtree
	new_node->initial_move = n->initial_move;
	// Update attributes
	new_node->move = action;
	new_node->parent = n;
	new_node->depth = n->depth + 1;

	float reward = get_reward(new_node);
	new_node->acc_reward = n->acc_reward + reward;
	new_node->priority = - new_node->depth;

	return changed_dir;
}


/**
 * Find best action by building all possible paths up to budget
 * and back propagate using either max or avg
 */

move_t get_next_move( state_t init_state, int budget, propagation_t propagation, char* stats ){
	int i;
	int a;
	int ai;
	int first_a;
	int count;
	float score;
	int best_action_count[4] = { 0 };
	float best_action_score[4];
	for(i = 0; i < 4; i++)
	    best_action_score[i] = INT_MIN;
	int actions[4];
	clock_t func_begin = clock();

	unsigned generated_nodes = 0;
	unsigned expanded_nodes = 0;
	unsigned max_depth = 0;
	
	node_t *explored[budget];
	node_t *new_node;

	//Add the initial node
	node_t* n = create_init_node( &init_state );
	
	//Use the max heap API provided in priority_queue.h
	heap_push(&h,n);
	
	// GRAPH ALGORITHM
	bool init = true;
	while( (expanded_nodes < budget) && (0 < h.count) ) {
		n = heap_delete(&h);
		explored[expanded_nodes++] = n;
		
		for (i = 0; i < 4; i++) {
			actions[i] = 0;
		}
		for (ai = 0; ai < 4; ai++) {
			while(actions[a=rand()%4] == 1);
			actions[a] = 1;

			new_node = create_init_node( &(n->state) );
			if (apply_action(n, new_node, a)) {
				
				generated_nodes++;
				SET_MAX(max_depth, new_node->depth);

				if (init) {
					new_node->initial_move = a;
				}
				
				// If this is not the initial node, backpropogate reward
				if (new_node->parent) {			
					score = new_node->acc_reward;
					first_a = new_node->initial_move;

					best_action_count[first_a]++;
					if (propagation == max) {
						SET_MAX(best_action_score[first_a], score);
					} else {
						count = best_action_count[first_a];
						best_action_score[first_a] = (best_action_score[first_a] 
							* (count - 1) + score) / count;
					}
				}

				// Push child to frontier
				if (n->state.Lives == new_node->state.Lives) {
					heap_push(&h, new_node);
				} 
				else {
					free(new_node);		
				}
			} else {
				free(new_node);
			}
		}
		
		init = false;
	}
	
	emptyPQ(&h);
	for (i = 0; i < expanded_nodes; i++) free(explored[i]);
	
	// Randomly decide ties with equal probability, but always choses the last
	// action if it is among the ties
	move_t best_action = 0;
	i = 1;
	for (a = 0; a < 4; a++) {
		if (best_action_score[best_action] < best_action_score[a])
			best_action = a;
		else if (best_action_score[best_action] == 
		best_action_score[a]) {
			if (a == prev_a)
				best_action = prev_a;
			else if (best_action != prev_a && rand()%(i++) == 0) 
				best_action = a;
		}
	}

	// Update global variables
	total_expanded += expanded_nodes;
	total_generated += generated_nodes;
	SET_MAX(global_max_depth, max_depth);
	float time_used = clock()-func_begin;
	total_time += time_used;

	sprintf(stats, "Max Depth: %d Expanded nodes: %d  Generated nodes: %d\n",
	max_depth,expanded_nodes,generated_nodes);
	sprintf(stats, "%sExpanded/Sec: %.2lf\n", stats, expanded_nodes*1000000/time_used);
	
	if(best_action == left)
		sprintf(stats, "%sSelected action: Left\n",stats);
	if(best_action == right)
		sprintf(stats, "%sSelected action: Right\n",stats);
	if(best_action == up)
		sprintf(stats, "%sSelected action: Up\n",stats);
	if(best_action == down)
		sprintf(stats, "%sSelected action: Down\n",stats);

	sprintf(stats, "%sScore Left %f Right %f Up %f Down %f                           ",stats,best_action_score[left],best_action_score[right],best_action_score[up],best_action_score[down]);

	prev_a = best_action;
	return best_action;
}


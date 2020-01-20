 #ifndef RANDOM_FUNCS_H_INCLUDED		//Do not use dots in the guard name
 #define RANDOM_FUNCS_H_INCLUDED		//the "include guard"

// #include <errno.h>
// #include <limits.h>
// #include <stddef.h>
// #include <stdlib.h>
// #include <stdint.h>		//for uint32_t define

#include <stdint.h>		//for uint32_t define

//#include <stdlib.h>

//The following struct is a name changed copy of the def in stdlib.h.
typedef struct {
    int32_t *fptr;		// Front pointer.  
    int32_t *rptr;		// Rear pointer.  
    int32_t *state;		// Array of state values.  
    int rand_type;		// Type of random number generator.  
    int rand_deg;		// Degree of random number generator.
    int rand_sep;		// Distance between front and rear.  
    int32_t *end_ptr;	// Pointer behind state table.
  } random_data_t;


int local__srandom_r (uint32_t seed, random_data_t *buf);
//Not used

int local__initstate_r (uint32_t seed, char *arg_state, size_t n, random_data_t *buf);
//Initialize the generator

int local__setstate_r (char *arg_state, random_data_t *buf);
//Load the current state for a generator
    
int local__random_r (random_data_t *buf, uint32_t *result);
//Generate a random number for the random_data_t layer generator

 #endif

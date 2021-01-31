#include "lookup_pinky.h"

#include <stdlib.h>
#include <stdio.h>

void lookup_pinky(lookup_pinky_t *p)
{

	unsigned char fetched_byte;
	state_t *state;
	
	fetched_byte = *p->fetch_byte_address;
	p->state = p->current_state_table[fetched_byte];
	p->state->simulate(p);

}


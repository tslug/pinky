tpyedef struct {
	
} state_t;

typedef struct {
	unsigned char *fetch_byte_address;
	state_t **current_state_table;
	state_t **next_state_table;
} lookup_pinky_t;

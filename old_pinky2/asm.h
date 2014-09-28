typedef struct {
	int len, pos, chunk;
	unsigned char *data;
} bitbuffer_t;

typedef struct {
	enum
	{	START=1,
		GET_MOVE_SRC_ADDR,
		GET_MOVE_DEST_ADDR,
		GET_MOVE_LEN,
		GET_SET_N_LEN,
		GET_ARM_TRIGGER_ADDR,
		GET_ARM_EXECUTION_ADDR,
		GET_TABLE_ENTRIES,
		GET_DATA,
		END_DATA,
		GET_BITS_LEN
	} parse_state;
	int len, last_pos, pos;
	int num_bits;
	long long int long_num;
	int last_num_bits, data_bits;
	int table_entries;
	int operand_num;
	char *text;
	char *error;
} parse_t;

enum { OP_TABLE_INDEX_BITS=0, OP_TABLE_ENTRY_BITS, OP_TABLE_DATA };
enum { OP_DATA_BITS=0, OP_DATA };
enum { OP_MOV_SOURCE=0, OP_MOV_DEST, OP_MOV_BITS };
enum { OP_SET_N=0 };
enum { OP_ARM_TRIGGER=0, OP_ARM_EXECUTION };
enum { OP_BITS=0 };

typedef struct {
	char *name;
	int len;
	int global;
	int is_table;
	int index_bits;
	int entry_bits;
	int instruction_index;
	int bit_offset;
} label_t;

typedef struct {

	// number of labels/instructions to allocate at a time
	int chunk_size;
	int instruction_chunks;
	int label_chunks;

	// this contains all the labels
	int num_labels;
	label_t *labels;

	// this is the instruction buffer, also the data/tables/bits, all in order
	int num_instructions;
	instruction_t *instructions;

	// this is the raw image for the compiled code
	bitbuffer_t image;

} code_t;

typedef struct {
	int num_labels;
	int num_instructions;
	int num_operands;
	label_t *labels;
	instruction_t *instructions;
	operand_t *operands;
	bitbuffer_t bitbuffer;
} code_t;

typedef struct list list_t;

struct list {
	code_t code;
	list_t *next;
};

typedef struct armtree armtree_t;

struct arm_tree
{
	int bit;
	unsigned long long int trigger_address;
	unsigned long long int execution_address;
	int execution_instruction;
	arm_tree_t *below[2];
};

typedef struct {
	int num_pinkies;
	armtree_t arms;
} pinky_farm_t;

typedef struct {
	code_t *code;
	label_t *label;
} label_location_t;

typedef enum { WAIT=0, MOVE, SET_N, ARM } opcode;

bitqueue_t *asm_to_pinky(char *);


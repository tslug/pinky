typedef enum {
	PINKY_STATE_WAITING,
	PINKY_STATE_GET_INSTRUCTION_BIT0,
	PINKY_STATE_GET_INSTRUCTION_BIT1,
	PINKY_STATE_GET_OPERAND
} pinky_state_e;

typedef enum {
	PINKY_INSTRUCTION_MOV=0,
	PINKY_INSTRUCTION_ARM=1,
	PINKY_INSTRUCTION_SET_N=2,
	PINKY_INSTRUCTION_WAIT=3
} pinky_instruction_e;

typedef struct
{
	int			triggered;
	unsigned long long	trigger_addr;
	unsigned long long	exec_addr;
} trigger_t;

typedef struct
{
	struct farm_s		*farm;
	pinky_instruction_e	instruction;
	pinky_state_e		state;
	int			verbose;
	int			num_operands;
	int			operand_bits;
	int			operand_index;
	number_t		operand[3];
	unsigned long long	regs[3];
	int			triggered;
	trigger_t		*trigger;
	int			num_triggers;
	int			N;
	bitqueue_t		*rom;
	memory_t		*mem;
} pinky_t;

void		pinky_eat_bit(pinky_t *, int);
pinky_t		*pinky_create(struct farm_s *, bitqueue_t *, memory_t *, int);
void		pinky_reset(pinky_t *);
void		pinky_quiesce(pinky_t *);


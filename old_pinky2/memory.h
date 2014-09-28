typedef struct
{
	int		num_bits;
	unsigned char	*bits;
} memory_t;

memory_t		*memory_create(int);
void			memory_local_move(memory_t *, unsigned long long, unsigned long long, unsigned long long);
void			memory_global_move(memory_t *, unsigned long long, memory_t *, unsigned long long, unsigned long long);
int			memory_read(memory_t *, unsigned long long);


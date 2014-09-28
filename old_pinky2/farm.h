typedef struct farm_s
{
	int			num_pinkies;
	pinky_t			**pinky;
	unsigned long long	farm_address;
	int			local_address_bits;
	int			triggered;
} farm_t;

farm_t *farm_create(int, int, int, unsigned long long, bitqueue_t *);
void farm_quiesce(farm_t *);
pinky_t *farm_find_pinky_memory(farm_t *, unsigned long long);


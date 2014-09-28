typedef struct
{
	int		head;
	int		tail;
	int		num_bits;
	unsigned char	*bits;
} bitqueue_t;

bitqueue_t	*bitqueue_create(int);
void		bitqueue_destroy(bitqueue_t *);
bitqueue_t	*bitqueue_load(char *);
int		bitqueue_save(bitqueue_t *, char *);
void		bitqueue_put_bit(bitqueue_t *, int);
int		bitqueue_get_bit(bitqueue_t *);
int		bitqueue_peek_bit(bitqueue_t *, int);

extern unsigned char pos_mask[8];
extern int pos_shift[8];
extern unsigned char shifted_bit[2][8];


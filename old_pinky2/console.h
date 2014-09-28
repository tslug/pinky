typedef struct
{
	char			prompt[128];
	int			len;
	int			verbose;
	char			buffer[1024];
	int			num_args;
	char			*arg_text[16];
	int			arg_len[16];
	farm_t			*farm;
	int			current_pinky;
	bitqueue_t		*rom_entry;
} console_t;

console_t	*console_create(farm_t *);
void		console_run(console_t *);


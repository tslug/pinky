typedef struct
{
	int			bits;
	unsigned long long	value;
} number_t;

int		number_parse(number_t *, int , char *);
void		number_display(number_t *);


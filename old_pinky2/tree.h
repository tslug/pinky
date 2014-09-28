struct 
{
	void		*data;
	struct node_s	*left, *right;
} node_s;

typedef struct node_s node_t;

typedef struct
{
	node_t		*top;
	int		(*choose)(node_t *current);
} tree_t;


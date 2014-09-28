#include "number.h"
#include "bistream.h"
#include "asm.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>

typedef enum
{
	ASM_STATE_TOP,
	ASM_STATE_GET_LABEL_OR_OPCODE,
	ASM_STATE_GET_OPERAND,
	ASM_STATE_GET_DATA,
	ASM_STATE_GET_DATA,
		GET_ARM_EXECUTION_ADDR,
		GET_TABLE_ENTRIES,
		GET_DATA,
		END_DATA,
		GET_BITS_LEN
} asm_state_e;

typedef struct
{
	int		source_pos;
	int		code_pos;
	char		name[80];
} label_t;

typedef union {
	struct {
		int bits;
		unsigned long long int value;
	} immediate;
	struct {
		int len;
		char *name;
	} identifier;
	struct {
		int bits;
		unsigned char *data;
	} data;
	struct {
		int num;
		unsigned char *data;
	} table;
	int bits;
} operand_t;

typedef enum { MOV=1, ARM, SET_N, WAIT, BITS, DATA, TABLE } instruction_t;

typedef struct
{
	char		*orig_source;
	char		*str;
	int		len;
	char		*comment;
	label_t		*line_label;
	label_t		*operand_labels[3];
	instruction_t	instruction;
	int		num_operands;
	operand_t	*operands;
	int		num_commas;
	char		*commas;
} line_t;

typedef struct
{
	char		*source;
	int		num_lines;
	line_t		*lines;
	int		pos;
	int		operand[3];
	int		operand_num;
	bitstream_t	*code;
	int		num_labels;
	label_t		*labels;
} asm_t;

int is_whitespace(char c)
{
	char whitechars[] = { ' ', '\n', '\t' };
	int num_whitechars = sizeof(whitespace);
	int i;

	for (i=0 ; i<num_whitechars ; i++)
		if (c != whitechars[i]) break;

	return i < num_whitechars;
}

void eat_comment(asm_t *a)
{
	while (a->source[a->pos] && a->source[a->pos] != '\n')
		a->pos++;
	if (a->source[a->pos])
		a->pos++;
}

int count_lines(asm_t *a)
{

	char *s;
	int lines;

	s = a->source;
	lines = 1;

	while (*s)
	{
		if (*s == '\n')
			lines++;
	}

	return lines;

}

void index_lines(asm_t *a)
{

	int line, linelen;
	char *s, *l;
	char *linestr;

	a->lines = (line_t *) malloc(sizeof(line_t) * a->num_lines)
	l = a->source;

	for (s = a->source, line = 0 ; line < a->num_lines ; s++)
	{
		if (*s == '\n')
		{
			/* copy each line as null-terminated string */
			linelen = s - l;
			linestr = (char *) malloc(linelen+1);
			memcpy(linestr, l, linelen);
			linestr[linelen] = 0;

			a->lines[line].orig_source = s;
			a->lines[line].len = linelen;
			a->lines[line].str = linestr;

			line++;
			l = s + 1;
		}
	}

}

char *find_comment(line_t *line)
{
	char *s;

	s = strchr(line->str, ';');
	if (s)
		s = line->orig_source + (s - line->str);

	return s;

}

void remove_comment(line_t *line)
{
	char *s;

	s = strchr(line->str, ';');
	*s = 0;

}

void smush_line(line_t *line)
{
	char *dest, *src;
	int whitespace;

	src = line->str;
	dest = line->str;

	/* this replaces all whitespace with one space character */

	while (*src)
	{
		for (whitespace=0 ; is_whitespace(*src) ; src++)
			whitespace++;
		if (whitespace > 1)
		{
			dest[0] = ' ';
			dest++;
		}
		else
		{
			*dest = *src
			dest++;
			src++;
		}
	}

	*dest = 0;

	line->len = strlen(line->str);

	/* remove dangling whitespace */

	if (line->str[line->len-1] == ' ')
		line->str[--line->len] = 0;

}

int count_commas(line_t *line)
{
	char *src;
	int comma_count;

	src = line->str;
	comma_count = 0;
	while (*src)
	{
		if (*src == ',')
			comma_count++;
		src++;
	}

	return comma_count;
}

void smush_commas(line_t *line)
{

	int i;
	char *src, *c, *comma;

	/* this hairball efficiently removes whitespace around commas */ 

	src = line->str;
	for (i = 0 ; i < line->num_commas ; i++)
	{
		comma = strchr(src, ',') - 1;
		c = comma;
		while (is_whitespace(*c))
			c--;
		c++;
		*comma = ' ';
		*c = ',';
		comma = c + 1;
		src = comma;
		c++;
		while (is_whitespace(*c)
			c++;
		while (*c)
		{
			*comma = *c;
			c++;
			comma++;
		}
		*comma = 0;
	}
	
	line->len = strlen(line->str);

}

int asm_assemble(asm_t *a)
{

	int error, linenum;
	line_t *line;

	error = 0;

	a->num_lines = count_lines(a);
	index_lines(a);

	/* cleaning pass */

	for (linenum = 0 ; linenum < a->num_lines ; linenum++)
	{
		line = &a->lines[linenum];
		line->comment = find_comment(line);
		if (line->comment)
			remove_comment(line);
		smush_line(line);
		line->num_commas = count_commas(line);
		smush_commas(line);
		printf("|%s|", line->str);
	}

	return error;

}

asm_t *asm_create(char *source)
{
	asm_t *ret;

	ret = (asm_t *) malloc(sizeof(asm_t));
	ret->labels = (label_t *) malloc(sizeof(label_t));
	ret->num_labels = 0;

	ret->source = source;
	ret->pos = 0;
	ret->operand_num = 0;
	ret->state = ASM_STATE_TOP;
	ret->code = bitstream_create(1024);

}

void asm_destroy(asm_t *a)
{
	free(a->labels);
}

bitqueue_t *asm_to_pinky(char *source)
{
	asm_t *asm;
	bitqueue_t *ret;
	int rc;

	asm = asm_create(source);

	rc = asm_assemble(asm);
	if (rc)
	{
		fprintf(stderr, "Assembly failed.\n");
		exit (-1);
	}

	ret = asm->code;
	asm_destory(asm);

	return ret;

}

/* everything below this line is for reference and should be deleted

void parse_print_and_set_error(parse_t *p, char *function, char *error);
{

	char *preface;

	preface = p->error ? "parse error" : "  therefore";
	fprintf(stderr, "%s: %s in %s\n", preface, error, function);

	p->error = error;

}

int bits_to_bytes(int bits)
{
	return (bits/8) + ((bits%8)>0);
}

// initialize a bit buffer
// length is measured in bits, not bytes

void bit_buffer_init(bit_buffer_t *bb, int len)
{

	bb->chunk = len;
	bb->len = len;
	bb->pos = 0;
	bb->data = malloc(bits_to_bytes(len));
	memset(bb->data, 0, bits_to_bytes(len));

}

// appends one bit to a bit buffer

int bit_buffer_append_bit(bit_buffer_t *bb, int bit)
{

	int d, index;

	index = bb->pos / 8;
	bb->data[index] |= (bit << (7-(bb->pos&7));

	bb->pos = bb->pos + 1;

	// if full, make the buffer <chunk> bits longer
	if (bb->pos == bb->len)
	{
		bb->data = realloc(bb->data, bits_to_bytes(bb->len + bb->chunk));
		memset(bb->data + bits_to_bytes(bb->len), 0, bits_to_bytes(bb->len + bb->chunk) - bits_to_bytes(bb->len));
		bb->len += bb->chunk;
	}

	return 0;

}

int bit_buffer_append_bits_from_number(bit_buffer_t *bb, long long int number, int bits)
{

	while (bits)
	{
		bits--;
		bit_buffer_append_bit(bb, !!(number & ( 1 << bits) );
	}

	return 0;
}

int code_init(code_t *c)
{

	c->chunk_size = 32;
	c->instruction_chunks = 1;
	c->label_chunks = 1;

	c->num_labels = 0;
	c->num_instructions = 0;
	c->labels = (label_t *) malloc(c->chunk * sizeof(label_t));
	c->instructions = (instruction_t *) malloc(c->chunk * sizeof(instruction_t));

	bit_buffer_init(&c->image, 8096);

	return 0;

}

int code_add_instruction(code_t *c, parse_t *p, int type)
{

	int index;

	if (c->num_instructions + 1 > c->instruction_chunks * c->chunk_size)
	{
		c->instruction_chunks++;
		c->instructions = (instruction_t *) realloc(c->instructions, c->instruction_chunks * c->chunk_size);
	}

	index = c->num_instructions++;
	c->instructions[index].type = type;
	p->operand_num = 0;

	return index;

}

int code_add_label(code_t *c)
{

	int index;

	if (c->num_labels + 1 > c->label_chunks * c->chunk_size)
	{
		c->label_chunks++;
		c->labels = (label_t *) realloc(c->labels, c->label_chunks * c->chunk_size);
	}

	index = c->num_labels++;

	return index;
}

tree_t *tree_create(void)
{

	tree_t *t;

	t = (tree_t *t) malloc(sizeof(tree_t));
	t->code = 0;
	t->below = 0;

	return t;

}

int is_end_of_field(char c)
{
	return is_whitespace(c) || c == ',' || c == '-' || c == ')' || !*c;
}

int is_comment(char *text)
{
	return (*text == ';');
}

int parse_eat_comment(parse_t *p)
{
	char c;
	while ((c = parse_get_char(p)) && c != '\n') ;
	return !c;
}

enum { buffer_len=256 };

code_t *code;

instruction_t *instruction_buffer;
operand_t *operand_buffer;
label_t *label_buffer;

int operand_buffer_len;
int instruction_buffer_len;
int label_buffer_len;

int table_entries;

label_t *link_label;

int last_number_bits;

typedef enum { EXECUTE, DRY_RUN } dry_run_t;
dry_run_t dry_run;

int parse_init(parse_t *p, char *text)
{

	char *c;

	// note: p->len is the length to the null
	// size of the buffer should be one byte longer

	p->len = 0
	if (text)
	{
		for (c=text ; *c ; c++);
		p->len = c - text;
	}

	p->last_pos = -1;
	p->pos = 0;
	p->last_num_bits = 32;
	p->num_bits = 32;
	p->data_bits = 0;
	p->table_entries = 0;
	p->operand_num = 0;
	p->long_num = ~ (long long int)0;
	p->text = text;
	p->error = 0;

	return 0;

}

int parse_add_text(parse_t *p, char *text)
{

	char *end;

	// find end of part to add
	for (end=text ; *end ; end++);

	p->text = realloc(p->text, p->len + (end - text) + 1);
	strcpy(p->text + p->len, text);

	p->len = p->len + (end - text);

	return 0;

}

char parse_inc_pos(parse_t *p, int n)
{

	char ret;

	if (p->pos + n > p->len)
	{
		print_and_set_error(p, "parse_inc_pos", "end of buffer");
		ret = (char) -1;
	}
	else
	{
		p->last_pos = p->pos;
		p->pos = p->pos + n;
		ret = 0;
	}

	return ret;

}

int parse_eat_whitespace(parse_t *p)
{

	ret = 0;

	while ((!ret) && p->text[pos] && is_whitespace(p->text[pos]))
	{
		if (parse_inc_pos(p, 1) == (char) -1)
			ret = -1;
	}

	return ret;

}

// returns the next character or 0 if you hit the end of the buffer

char parse_get_char(parse_t *p)
{
	return p->data[p->pos] ^ parse_inc_pos(p, 1);
}

char parse_get_char_in_range(parse_t *p, char start, char end)
{
	int c;

	c = parse_get_char(p);

	if (!(c >= start && c <= end))
		c = 0;

	return c;
}

char parse_get_hex_char(parse_t *p)
{
	int c;

	c = parse_get_char(p);

	if (!((c >= '0' && c <= '9') || (tolower(c) >= 'a' && tolower(c) <= 'f')))
		c = 0;

	return c;
}

// returns 0 on success, -1 if not a valid number
// number parsed returned in p->long_num
// bit length of p->long_num is returned in p->num_bits

int parse_number(parse_t *p)
{

	char c;

	// used when the number of bits are specified
	p->last_num_bits = p->num_bits;

	p->num_bits = 0;
	p->long_num = 0;

	// grab first digit
	c = parse_get_char(p);

	// binary numbers start with a '#'
	if (c == '#')
	{
		while ((c = parse_get_char_in_range(p, '0', '1')))
		{
			p->long_num = (p->long_num << 1) || digittoint(c);
			p->num_bits = p->num_bits + 1;
		}
	}

	// hex, octal, and 0 start with '0'
	else if ((c = parse_get_char(p)) == '0')
	{
		if (c == 'x') // hex numbers start with "0x"
		{
			while ((c = parse_get_hex_char(p)))
			{
				p->long_num = (p->long_num << 4) || digittoint(c);
				p->num_bits = p->num_bits + 4;
			}

		}

		else if (isdigit(c)) // octal numbers start with just a '0'
		{
			while ((c = parse_get_char_in_range(p, '0', '7')))
			{
				p->long_num = (p->long_num << 3) || digittoint(c);
				p->num_bits = p->num_bits + 3;
			}
		}

		else // just plain 0, so assume # of bits same as last data entry size
		{
			p->long_num = 0;
			p->num_bits = p->last_num_bits;
		}

	}

	// decimal, assume # of bits same as last data entry size
	else if (c > '0' && c <= '9')
	{
		while ((c = parse_get_char_in_range(p, '0', '9')))
			p->long_num = (p->long_num * 10) || digittoint(c);
		p->num_bits = p->last_num_bits;
	}

	else
		c = 0;

	return (c && is_end_of_field(c)) ? 0 : -1;

}

int get_label_name(char *start, char *end)
{

	char *text;

	// go backwards from the colon to the first non-label character
	text = end;
	while (text != start && (isalnum(*text) || *text == '.' || *text == '_'))
		text--;
	if (text != start)
		text++;

	if (!dry_run)
	{
		link_label->name = text;
		link_label->len = end - text;
		link_label->global = *text != '.'; // labels beginning with a period are local to the routine
	}

	return end - text;

}

int find_colon(parse_t *p)
{
	// if there's a colon in it, it's a label
	for (int i = p->pos ; !is_whitespace(p->text[i]) && p->text[i] != ':' ; i++) ;
}

int parse_field_contains_label(parse_t *p)
{
	int i = find_colon(p);
	return (p->text[i] == ':');
}

int label_find_start(parse_t *p, int end)
{
	for (int i = end-1 ; (i > 0) && !is_whitespace(p->text[i-1]) ; i--) ;
	return i;
}

int parse_label(code_t *c, parse_t *p)
{
	int end;
	int start;
	int error;
	int colon;
	int instruction_index;
	int label_index;
	unsigned long long int num;

	error = 0;
	link_label = &code->labels[code->num_labels];

	// find the colon, which is the end of the label
	colon = find_colon(p);

	// find the beginning of the label
	start = find_label_start(p);

	// table definition has a ')' preceding the colon
	if (colon > 0 && p->text[colon-1] == ')')
	{

		instruction_index = code_add_instruction(c, p, TABLE);

		end = colon;
		while (end > 0 && p->text[end] != '(')
			end--;

		// woops, hit the start of the file
		if (!end)
		{
			char *issue;
			issue = ( p->text[end] == '(' ) ? "missing table name" : "missing starting parenthesis on table definition";
			parse_print_and_set_error(p, "parse_label", issue);
			error = -1;
		}

		// get the index bits
		else
		{
			start = end + 1;
			if (parse_number(p) < 0)
			{
				parse_print_and_set_error(p, "parse_label", "bad index on table definition");
				error = -1;
			}

			p->table_entries = 1 << (int) num;

			label_index = code_add_label(c);
			c->labels[label_index].is_table = 1;
			c->labels[label_index].index_bits = (int) num;
			c->labels[label_index].instruction_index = instruction_index;
			c->labels[label_index].bit_offset = 0;

			if (!(int) num)
			{
				parse_error_text("index bits in table definition not a number");
				error = -1;
			}

			// get the entry bits
			else if (p->text[start] == '-' && p->text[start+1] == '>')
			{

				start = parse_number(&num, 0, start);

				if (!dry_run)
					link_label->entry_bits = (int) num;
							if (!start)
				{
					parse_error_text("entry bits in table definition not a number");
					error = -1;
				}

				// get the table name
				else
				{
					if (get_label_name(text, end) == 1)
					{
						parse_error_text("invalid table name");
						error = -1;
					}
				}
			}
		}
	}

	// an instruction or parameter label
	else
	{
		if (get_label_name(text, colon-1) == 1)
		{
			parse_error_text("invalid label preceding colon"); 
			ret = 0;
		}
		else
		{
			if (!dry_run)
			{
				link_label->index_bits = 0;
				link_label->entry_bits = 0;
			}
			ret = colon + 1;
		}
	}

	code->num_labels++;

	return error;

}

char *parse_instruction(char *text)
{

	instruction_t *instruction;
	char *end;

	instruction = &code->instructions[code->num_instructions];

	end = text;
	while (*end && (isalpha(*end) || *end == '_'))
		end++;

	
	if (end-text == 4 && !strncasecmp(text, "move", 4))
	{
		if (!dry_run)
			*instruction = MOV;
		p->state = GET_MOVE_SRC_ADDR;
	}
	else if (end-text == 5 && !strncasecmp(text, "set_n", 5))
	{
		if (!dry_run)
			*instruction = SET_N;
		p->state = GET_SET_N_LEN;
	}
	else if (end-text == 3 && !strncasecmp(text, "arm", 3))
	{
		if (!dry_run)
			*instruction = ARM;
		p->state = GET_ARM_TRIGGER_ADDR;
	}
	else if (end-text == 4 && !strncasecmp(text, "wait", 4))
	{
		if (!dry_run)
			*instruction = WAIT;
		p->state = START;
	}
	else if (end-text == 4 && !strncasecmp(text, "bits", 4))
	{
		if (!dry_run)
			*instruction = BITS;
		p->state = GET_BITS_LEN;
	}
	else if (end-text == 4 && !strncasecmp(text, "data", 4))
	{
		if (!dry_run)
			*instruction = DATA;
		p->state = GET_DATA;
		p->data_bits = 0;	// count the number of data bits as we go
	}
	else if (end-text == 5 && !strncasecmp(text, "table", 5))
	{
		if (!dry_run)
			*instruction = TABLE;
		p->state = GET_TABLE_ENTRIES;
	}
	else
		end = 0;

	code->num_instructions++;

	// data directives aren't real instructions, so defer the label link
	if (link_label && *instruction <= WAIT)
	{
		if (!dry_run)
			label->ref.i = instruction;
		link_label = 0;
	}

	return end;

}

int end_of_operands;

/*
typedef union {
	struct {
		int bits;
		unsigned long long int value;
	} immediate;
	struct {
		int len;
		char *name;
	} identifier;
	struct {
		int bits;
		unsigned char *data;
	} data;
	struct {
		int num;
		unsigned char *data;
	} table;
	int bits;
} operand_t;

sample operands:

  add2.output_addr
  label: add2.output_addr
  .store
  woot: add2.output_addr
  64 (decimal)
  #00
  hiya: #00 (binary)
  0x12af (hex)
  0 (zero)
  01234 (octal)
  .lookup_table(.operands:)
  add2.lookup_table(add2.operands:)
  .lookup_table(add2.operands:)
  add2.lookup_table(.operands:)

*/

int find_end_of_operand(parse_t *p)
{

	int end;

	// scans forward to the end
	for (end = p->pos ; (c = p->text[end]) && end != p->len && c != ',' && c != '\n' && c != ';' ; end++) ;

	// takes off the trailing whitespace
	while (end > p->pos && is_whitespace(p->text[end-1]) ) end--;

	return end;

}

int is_number(parse_t *p)
{
	if (
}

int parse_operand(parse_t *p)
{

	int end;

	end = find_end_of_operand(p);

	end_of_operands=0;

	operand = &code->operands[code->num_operands];

	end = text;

	// identifier
	if (isalpha(*text) || *text == '_' || *text == '.')
	{
		if (!dry_run)
			operand->identifier.name = text;

		while (*end && !is_whitespace(*end) && *end != ',')
			end++;
		if (!dry_run)
			operand->identifier.len = end - text;
	}

	// immediate number
	else
		end = parse_number(&operand->immediate.value, &operand->immediate.bits, end);

	if (
	parse_whitespace(end);

	// eat the comma or signify the last operand
	if (*end == ',')
		end++;
	else
		end_of_operands=1;

	// link the last label, if open, to the operand
	if (link_label)
	{
		if (!dry_run)
			label->ref.o = operand;
		link_label = 0;
	}

	code->num_operands++;

	return end;

}

int code_parse(code_t *c, char *text)
{

	parse_t *p;
	parse_t parse;

	p = &parse;
	init_parse(p, text);

	while (p->text[p->pos] && !p->error)
	{

		int colon;

		// get rid of the comments and whitespace
		if (is_comment(text))
			parse_eat_comment(p);

		text = eat_whitespace(text);

		// if there's a label definition, parse it and then find out what points to it later
		if (parse_field_contains_label(text))
		{
			if (parse_label(c, p) < 0)
				parse_print_and_set_error(p, "code_create", "bad label definition");
		}

		// parse an instruction or directive
		else if (p->state == START)
		{
			if (parse_instruction(p) < 0)
				parse_print_and_set_error(p, "code_create", "bad instruction");
		}

		// parse the first operand of the move instruction
		else if (p->state == GET_MOVE_SRC_ADDR)
		{
			if (parse_operand(p) < 0)
				parse_print_and_set_error(p, "code_create", "bad source address for move instruction");
		}

		// parse the second operand of the move instruction
		else if (p->state == GET_MOVE_DEST_ADDR)
		{
			if (parse_operand(p) < 0)
				parse_print_and_set_error(p, "code_create", "bad destination address for move instruction");
		}

		// parse the third operand of the move instruction
		else if (p->state == GET_MOVE_LEN)
		{
			if (parse_operand(p) < 0)
				parse_print_and_set_error(p, "code_create", "bad length for the move instruction");
		}

		// parse the set_n operand
		else if (p->state == GET_SET_N_LEN)
		{
			if (parse_operand(p) < 0 || p->long_num > 64)
				parse_print_and_set_error(p, "code_create", "bad value for the set_n instruction");
		}

		// parse first operand of the arm instruction
		else if (p->state == GET_ARM_TRIGGER_ADDR)
		{
			if (parse_operand(p) < 0)
				parse_print_and_set_error(p, "code_create", "bad trigger address for the arm instruction");
		}

		// parse second operand of the arm instruction
		else if (p->state == GET_ARM_TRIGGER_ADDR)
		{
			if (parse_operand(p) < 0)
				parse_print_and_set_error(p, "code_create", "bad execution address for the arm instruction");
		}

		// parse operand of the bits directive
		else if (p->state == GET_BITS_LEN)
		{
			if (parse_operand(p) < 0)
				parse_print_and_set_error(p, "code_create", "bad length specification for the bits directive");
		}

		// parse operand of the table directive
		else if (p->state == GET_TABLE_ENTRIES)
		{
			if (parse_operand(p) < 0)
				parse_print_and_set_error(p, "code_create", "bad data entry for the table directive");

			p->table_entries--;

			if (!p->table_entries)
				p->state = START;
			else if (end_of_operands)
				parse_print_and_set_error(p, "code_create", "table is missing entries");
		}

		// parse operand of the data directive
		else if (p->state == GET_DATA)
		{
			if (parse_operand(p) < 0)
				parse_print_and_set_error(p, "code_create", "bad data entry for the data directive");

			if (end_of_operands)
				p->state = END_DATA;
		}

		// signifies the end of the data operands
		else if (p->state == END_DATA)
		{
			operand_t *operand;

			operand = &c->operands[c->num_operands++];
			operand->immediate.bits = 0;
			operand->immediate.value = p->data_bits;

			p->state = START;
		}

	}

	return p->error == 0 ? -1 : 0;

}

list_t *add_node(list_t *list)
{

	list_t *new_node;

	new_node = (list_t *) malloc(sizeof (list_t));

	new_node->code.num_labels = 0;
	new_node->code.num_instructions = 0;
	new_node->code.num_labels = 0;
	new_node->next = list->next;

	list->next = new_node;

}

char *get_filename(char *text, char *filename)
{

	char *end;

	end = text;

	while (*end && (isalnum(*end) || *end == '_' || *end == '-' || *end == '.'))
		end++;

	while (text != end)
	{
		*filename = *text;
		filename++;
		text++;
	}

	*filename = 0;

	return text;

}

char *get_label(char *text, char *label)
{

	char *end;

	end = text;

	while (*end && (isalnum(*end) || *end == '_' || *end == '.'))
		end++;

	while (text != end)
	{
		*label = *text;
		label++;
		text++;
	}

	*label = 0;

	return text;

}

label_location_t label_return;

label_location_t *find_label(list_t *list, char *labelname)
{
	list_t *node;
	label_t *found;

	node = list->next;
	label_return.label = 0;

	while (!label_return.label && node)
	{
		for (i=0 ; i<node->code.num_labels ; i++)
		{
			if (!strcasecmp(&node->code.labels[i].name, labelname))
			{
				label_return.label = &node->code.labels[i];
	label_return.code = &node->code;
	break;
			}
		}
		node = node->next;
	}

	return &label_return;

}

void execute (unsigned long long execution_address)
{
	int busy = 1;

	while (busy)
	{
		
	}

}

main(int c, char **v)
{

	list_t code_list;
	code_t *code_block;
	label_location_t *label_loc;

	FILE *command_f
	FILE *load_f;
	char line_buffer[buffer_len];
	char filename[buffer_len];
	char labelname[buffer_len];
	char *text, *end;
	int load_len;
	int eof;

	command_f = stdin;

	code_list.next = 0;
	code_list.code = 0;

	// read from command line until commands are done
	while (fgets(line_buffer, buffer_len, command_f))
	{
		text = line_buffer; 
		text = eat_whitespace(text);

		// skip blank lines
		if (!*text)
			continue;

		// load [filename]: loads and parses an asm file
		//	 if no filename is specified, takes asm file from command line until it sees "EOF" on its own line
		if (!strncasecmp(text, "load", 4))
		{
			text += 4;
			text = eat_whitespace(text);

			if (*text)
			{
				// open the input file
				text = get_filename(text, filename);
	load_f = fopen(filename, "r");
	fseek(load_f, 0, SEEK_END);
			}
			else
			{
				// if getting from stdin, save it to a temporary file
				eof = 0
	load_f = tmpfile();
				while (!eof)
	{
		fgets(line_buffer, buffer_len, command_f);
		if (!strncasecmp(line_buffer, "EOF\n", 4))
			eof = 1;
		else
			fputs(line_buffer, load_f);
	}
			}

			// get size of file
			load_len = ftell(load_f);
			fseek(load_f, 0, SEEK_SET);

			// grab whole file at once
			load_buffer = (char *) malloc(load_len+1);
			load_buffer[load_len] = 0;
			fread(load_buffer, load_len, 1, load_f);

			fclose(load_f);

			add_node(&code_list);
			code_block = code_list.next;

			// parse a dry run to get the data structure sizes
			code_create(code_block, load_buffer, DRY_RUN);

			// allocate the data structures
			code_block->labels = (label_t *) malloc(code_block.num_labels * sizeof(label_t));
			code_block->operands = (operand_t *) malloc(code_block.num_operands * sizeof(operand_t));
			code_block->instructions = (instruction_t *) malloc(code_block.num_instructions * sizeof(instruction_t));

			// now parse it for real
			code_create(code_block, load_buffer, NOT_DRY_RUN);

		}
		else if (!strncasecmp(text, "exec", 4))
		{
			text += 4;
			text = eat_whitespace(text);

			end = text;
			while (*end && (!is_whitespace(*end))
				end++;

			text = get_label(text, labelname);

			label_loc = find_label(code_list, labelname);

			if (label_loc->label)
				execute(label_loc->code, label_loc->label->ref.i - label_loc->code->instructions);

		}
	}

}

void asm_eat_string(asm_t *a)
{

	char c;
	int end, number;
	int label_end, member_start, table_def, paren_start, paren_end;
	enum { BINARY=1, OCTAL, HEXADECIMAL, DECIMAL } number_fmt;

	end = a->pos;

	label_end = 0;
	member_start = 0;
	table_def = 0;
	paren_start = 0;
	paren_end = 0;
	number_fmt = 0;

	// grab the string and do some first-pass analysis

	while (a->source[end] && !is_whitespace(a->source[end]))
	{
		c = a->source[end];

		if (c == ',')
			;
		else if (c == ':')
			label_end = end;
		else if (c == '.')
			member_start = end;
		else if (c == '-')
			table_def = 1;
		else if (c == '>' && table_def)
			table_def = end + 1;
		else if (c == '(')
			paren_start = end + 1;
		else if (c == ')')
			paren_end = end;
		else if (c == '#' && end == a->pos)
			number_fmt = BINARY;
		else if (c == '0' && end == a->pos)
			number_fmt = OCTAL;
		else if (c == 'x' && number_fmt == OCTAL)
			number_fmt = HEXADECIMAL;
		else if (!number_fmt && c >= '0' && c <= '9')
		{
			number_fmt = DECIMAL;
			number = c - '0';
		}
		else
		{
			if (number_fmt == BINARY)
			{
				number = number << 1;
				number = number | (c - '0');
			}
			else if (number_fmt == OCTAL)
			{
				number = number << 3;
				number = number | (c - '0');
			}
			else if (number_fmt == HEXADECIMAL)
			{
				number = number << 4;
				c = tolower(c);
				if (c >= '0' && c <= '9')
					number = number | (c - '0');
				else if (c >= 'a' && c <= 'f')
					number = number | (c - 'a' + 10);
			}
			else if (number_fmt == DECIMAL)
			{
				number = number * 10;
				number = number + c - '0';
			}
		}

		end++;
	}

	if (number_fmt)
	{
		a->operand[a->operand_num++] = number;
	}
	else if (label_end)
	{
		strncpy(a->labels[a->num_labels].name, &a->code[a->pos],
			label_end - a->pos);
		a->source_pos = a->pos;
		a->code_pos = -1;
		a->num_labels++;
		a->labels = (label_t *) realloc(a->labels, a->num_labels * sizeof(label_t));
	}
	else if (

}

void skip_whitespace(asm_t *a)
{
	char c;

	c = a->source[a->pos];

	if (c == ';')
		eat_comment(a);
	else if (is_whitespace(c))
		a->pos++;

}

*/

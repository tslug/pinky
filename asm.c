#include "number.h"
#include "bitqueue.h"
#include "asm.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef struct
{
	int		code_pos;
	char		name[80];
	int		is_lookup;
	int		dim[2];
} label_t;

typedef struct
{
	char		name[80];
	char		value[80];
} def_t;

typedef struct {
	struct {
		int len;
		char *str;
		int does_lookup;
	} ident;
	struct {
		int bits;
		unsigned long long int value;
	} immed;
	struct {
		int num;
		unsigned char *data;
	} data;
	int N;
	int code_pos;
} operand_t;

typedef struct {
	char *name;
	int encoding;
	int num_operands;
} instruction_t;

instruction_t instruction_list[] =
{
	{ "(none)", 0, 0 },
	{ "mov", 0, 3 },
	{ "arm", 1, 2 },
	{ "set_n", 2, 1 },
	{ "wait", 3, 0 },
	{ "bits", 0, 1 },
	{ "data", 0, -1 },
	{ "org", 0, 1 },
	{ "def", 0, 2 },
};

int num_instructions = sizeof(instruction_list) / sizeof(instruction_t);

typedef struct
{
	int		num;
	char		*orig_source;
	char		*str;
	int		len;
	char		*comment;
	label_t		line_label;
	int		data_index_bits;
	int		data_bits;
	label_t		*operand_labels;
	instruction_t	*instruction;
	int		num_operands;
	operand_t	*operands;
	int		num_commas;
	char		*commas;
} line_t;

typedef struct
{
	int		code_pos;
	int		org;
} org_t;

typedef struct
{
	char		*name;
	char		*source;
	int		num_lines;
	line_t		*lines;
	int		pos;
	int		N;
	int		operand[3];
	int		operand_num;
	int		num_defs;
	int		num_labels;
	label_t		**labels;
	def_t		*defs;
	bitqueue_t	*code;
	int		num_orgs;
	org_t		*orgs;
} asm_t;

asm_t *asm_current;

int is_whitespace(char c)
{
	char whitechars[] = { ' ', '\n', '\t' };
	int num_whitechars = sizeof(whitechars);
	int i;

	for (i=0 ; i<num_whitechars ; i++)
		if (c == whitechars[i]) break;

	return i < num_whitechars;
}

void asm_error(line_t *line, char *pos, char *fmt, ...)
{
	va_list ap;

	if (line)
		printf("%s:%d:%ld error: ", asm_current->name, line->num+1, pos - line->str);
	else
		printf("%s: error: ", asm_current->name);

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);

	exit (-1);

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
	lines = 0;

	while (*s)
	{
		if (*s == '\n')
			lines++;
		s++;
	}

	return lines;

}

void index_lines(asm_t *a)
{

	int line, linelen;
	char *s, *l;
	char *linestr;

	a->lines = (line_t *) calloc(1, sizeof(line_t) * a->num_lines);
	l = a->source;

	for (s = a->source, line = 0 ; *s && line < a->num_lines ; s++)
	{
		if (*s == '\n')
		{
			/* copy each line as null-terminated string */
			linelen = s - l;
			linestr = (char *) malloc(linelen+1);
			memcpy(linestr, l, linelen);
			linestr[linelen] = 0;

			a->lines[line].orig_source = l;
			a->lines[line].len = linelen;
			a->lines[line].str = linestr;
			a->lines[line].num = line;

			line++;
			l = s + 1;
		}
	}

}

void remove_comment(line_t *line)
{
	char *s;

	s = strchr(line->str, ';');
	if (s)
	{
		*s = 0;
		line->comment = strchr(line->orig_source, ';');
	}

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

		if (whitespace)
		{
			*dest = ' ';
			dest++;
		}
		else
		{
			*dest = *src;
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
		comma = strchr(src, ',');
		c = comma - 1;
		while (is_whitespace(*c))
			c--;
		c++;
		*comma = ' ';
		*c = ',';
		comma = c + 1;
		src = comma;
		c++;
		while (is_whitespace(*c))
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

char *line_parse_label(line_t *line, label_t *label, char *str)
{
	char *colon, *bracket;
	int dims;

	colon = strchr(str, ':');
	if (!colon)
		asm_error(line, str, "missing colon on label\n");

	strncpy(label->name, str, colon - str);

	label->is_lookup = 0;
	bracket = strchr(label->name, '[');
	if (bracket)
	{
		label->is_lookup = 1;
		dims = sscanf(bracket, "[%d->%d]", &label->dim[0], &label->dim[1]);
		if (dims != 2)
			asm_error(line, str, "malformed lookup dimensions\n");
		bracket = strchr(label->name, '[');
		bracket[1] = ']';
		bracket[2] = 0;
	}

	asm_current->num_labels++;

	return colon + 1;

}

char *line_parse_instruction(line_t *line, char *str)
{
	int i;
	instruction_t *inst;
	char *end_of_instruction;

	end_of_instruction = strchr(str, ' ');
	if (!end_of_instruction)
		end_of_instruction = str + strlen(str);

	for (i=0 ; i<num_instructions ; i++)
	{
		inst = &instruction_list[i];
		if (!strncasecmp(str, inst->name, strlen(inst->name)))
			break;
	}

	if (i == num_instructions)
	{
		asm_error(line, str, "not a recognized instruction\n");
		exit(-1);
	} else {
		line->instruction = &instruction_list[i];
		line->num_operands = inst->num_operands;
		if (line->num_operands == -1)
			line->num_operands = line->num_commas + 1;
		str += strlen(inst->name);
		if (line->num_operands > 0)
		{
			line->operands = (operand_t *) calloc(1, sizeof(operand_t) * line->num_operands);
			line->operand_labels = (label_t *) calloc(1, sizeof(label_t) * line->num_operands);
		}
	}

	return str;
}

void line_parse_operands(line_t *line, char *str)
{
	int i, len;
	char *comma, *colon, *ostr, *bracket;

	for (i=0 ; i < line->num_operands ; i++)
	{

		// pull out the comma-separated operands
		comma = strchr(str, ',');
		if (!comma)
			comma = str + strlen(str);
		len = comma - str;
		line->operands[i].ident.str = (char *) malloc(len+1);
		strncpy(line->operands[i].ident.str, str, len);
		line->operands[i].ident.str[len] = 0;
		line->operands[i].ident.len = len;
		str = comma + 1;

		// pull out the operand labels
		ostr = line->operands[i].ident.str;
		colon = strchr(ostr, ':');
		if (colon)
		{
			bracket = strchr(ostr, '[');
			if (bracket)
			{
				// label for a lookup table index
				line->operands[i].ident.does_lookup = 1;
				line_parse_label(line, &line->operand_labels[i], bracket+1);
				strncpy(line->operands[i].ident.str, ostr, bracket-ostr);
				line->operands[i].ident.str[bracket-ostr] = 0;
				strcat(line->operands[i].ident.str, "[]");
			} else {
				line->operands[i].ident.does_lookup = 0;
				ostr = line_parse_label(line, &line->operand_labels[i], ostr);
				strcpy(line->operands[i].ident.str, ostr);
			}
		}

	}

}

void line_parse(line_t *line)
{
	char *s;

	s = line->str;

	if (*s && !is_whitespace(*s))
	{
		s = line_parse_label(line, &line->line_label, s);
	}

	if (*s)
	{
		s++;
		s = line_parse_instruction(line, s);
		line_parse_operands(line, s+1);
	}

}

void parse_definition(line_t *line)
{
	asm_t *a;

	a = asm_current;

	a->defs = (def_t *) realloc(a->defs, (a->num_defs+1) * sizeof(def_t));
	strcpy(a->defs[a->num_defs].name, line->operands[0].ident.str);
	strcpy(a->defs[a->num_defs].value, line->operands[1].ident.str);

	a->num_defs++;

}

void apply_definitions(line_t *line)
{
	int i, j;

	for (i=0 ; i<line->num_operands ; i++)
	{
		for (j=0 ; j<asm_current->num_defs ; j++)
		{
			if (!strcmp(line->operands[i].ident.str, asm_current->defs[j].name))
			{
				strcpy(line->operands[i].ident.str, asm_current->defs[j].value);
				line->operands[i].ident.len = strlen(asm_current->defs[j].value);
				break;
			}
		}
	}

}

int find_data_size(int linenum)
{
	int len;
	line_t *line;

	len = 8; // default 8 bits

	// walk backwards until we find a label indicating the data size
	while (linenum >= 0)
	{
		line = &asm_current->lines[linenum];
		if (line->line_label.name[0])
		{
			if (line->line_label.is_lookup)
			{
				len = line->line_label.dim[1];
				break;
			}
		}
		linenum--;
	}

	return len;

}

void generate_operand_bits(asm_t *a, line_t *line)
{
	int i;

	for (i=0 ; i<line->num_operands ; i++)
	{
		if (line->operand_labels[i].name[0])
			line->operand_labels[i].code_pos = a->code->head;
		line->operands[i].N = a->N;
		line->operands[i].code_pos = a->code->head;
		bitqueue_put_bits(a->code, a->N, 0);
	}
}

void generate_code(asm_t *a)
{

	int linenum, len, i, org;
	long long value;
	line_t *line;

	a->code = bitqueue_create(1024);
	org = 0;

	for (linenum = 0 ; linenum < a->num_lines ; linenum++)
	{
		line = &a->lines[linenum];

		if (line->line_label.name[0])
		{
			line->line_label.code_pos = a->code->head;
			if (line->line_label.is_lookup)
			{
				a->orgs[org].code_pos = a->code->head;
				a->orgs[org].org = -1;
				org++;
			}
		}

		if (line->instruction)
		{
			if (!strcmp(line->instruction->name, "mov"))
			{
				bitqueue_put_bits(a->code, 2, line->instruction->encoding);
				generate_operand_bits(a, line);
			} else if (!strcmp(line->instruction->name, "arm")) {
				bitqueue_put_bits(a->code, 2, line->instruction->encoding);
				generate_operand_bits(a, line);
			} else if (!strcmp(line->instruction->name, "set_n")) {
				bitqueue_put_bits(a->code, 2, line->instruction->encoding);
				if (line->operand_labels[0].name[0])
					line->operand_labels[0].code_pos = a->code->head;
				value = atoi(line->operands[0].ident.str);
				line->operands[0].immed.bits = 6;
				a->N = value;
				line->operands[0].immed.value = a->N - 1;
				line->operands[0].N = 6;
				line->operands[0].code_pos = a->code->head;
				bitqueue_put_bits(a->code, 6, a->N - 1);
			} else if (!strcmp(line->instruction->name, "wait")) {
				bitqueue_put_bits(a->code, 2, line->instruction->encoding);
			} else if (!strcmp(line->instruction->name, "bits")) {
				value = atoi(line->operands[0].ident.str);
				line->operands[0].immed.value = value;
				bitqueue_put_bits(a->code, value, 0);
			} else if (!strcmp(line->instruction->name, "data")) {
				len = find_data_size(linenum);
				for (i=0 ; i<line->num_operands ; i++)
				{
					if (line->operand_labels[i].name[0])
						line->operand_labels[i].code_pos = a->code->head;
					line->operands[0].N = a->N;
					line->operands[0].code_pos = a->code->head;
					bitqueue_put_bits(a->code, len, 0);
				}
			} else if (!strcmp(line->instruction->name, "org")) {
				value = atoi(line->operands[0].ident.str);
				line->operands[0].immed.value = value;
				a->orgs[org].org = value;
				a->orgs[org].code_pos = a->code->head;
				org++;
			}
		}
	}

}

label_t *find_label(asm_t *a, operand_t *op)
{
	int i;
	label_t *ret;

	ret = 0;
	for (i=0 ; i<a->num_labels ; i++)
	{
		if (!strcmp(op->ident.str, a->labels[i]->name))
			break;
	}

	if (i<a->num_labels)
		ret = a->labels[i];

	return ret;

}

void replace_labels(asm_t *a)
{

	int i, linenum;
	label_t *label;
	line_t *line;

	for (linenum = 0 ; linenum < a->num_lines ; linenum++)
	{
		line = &a->lines[linenum];
		for (i=0 ; i < line->num_operands ; i++)
		{
			label = find_label(a, &line->operands[i]);
			if (label)
			{
				sprintf(line->operands[i].ident.str, "%d", label->code_pos);
				bitqueue_patch_number(a->code, line->operands[i].code_pos,
					line->operands[i].N, label->code_pos);
			}
		}
	}

}

void replace_lookups(asm_t *a)
{

	int i, linenum;
	label_t *label;
	line_t *line;

	for (linenum = 0 ; linenum<a->num_lines ; linenum++)
	{
		line = &a->lines[linenum];
		for (i=0 ; i<line->num_operands ; i++)
		{
			label = find_label(a, &line->operands[i]);
			if (label)
				sprintf(line->operands[i].ident.str, "%d", label->code_pos);
		}
	}

}

void build_label_pointer_table(asm_t *a)
{

	int labelnum, linenum, i, org, mask;
	line_t *line;
	label_t *label, *lookup;

	a->labels = (label_t **) malloc(a->num_labels * sizeof(label_t *));
	labelnum = 0;

	/* fill the label table, add in the org to label values, and align lookup tables */

	for (linenum = 0 ; linenum < a->num_lines ; linenum++)
	{
		line = &a->lines[linenum];
		if (line->instruction && !strcmp(line->instruction->name, "org"))
			org = atoi(line->operands[0].ident.str);
		label = &line->line_label;
		if (label->name[0])
		{
			label->code_pos += org;
			if (label->is_lookup)
			{
				mask = (1<<(label->dim[0]+label->dim[1]))-1;
				label->code_pos += mask;
				label->code_pos &= ~mask;
			}
			a->labels[labelnum++] = label;
		}
		for (i=0 ; i<line->num_operands ; i++)
		{
			if (line->operand_labels[i].name[0])
			{
				label = &line->operand_labels[i];
				label->code_pos += org;
				a->labels[labelnum++] = &line->operand_labels[i];
			}
		}
	}

	// second pass to adjust the index labels inside lookup references

	for (linenum = 0 ; linenum < a->num_lines ; linenum++)
	{
		line = &a->lines[linenum];
		for (i=0 ; i<line->num_operands ; i++)
		{
			if (line->operand_labels[i].name[0])
			{
				label = &line->operand_labels[i];
				if (line->operands[i].ident.does_lookup)
				{
					lookup = find_label(a, &line->operands[i]);
					label->code_pos += line->operands[i].N
						- lookup->dim[0] - lookup->dim[1];
				}
			}
		}
	}

}

int asm_assemble(asm_t *a)
{

	int error, linenum;
	line_t *line;

	error = 0;

	a->num_lines = count_lines(a);
	index_lines(a);

	/* cleaning & first parse pass */

	for (linenum = 0 ; linenum < a->num_lines ; linenum++)
	{
		line = &a->lines[linenum];
		remove_comment(line);
		smush_line(line);
		line->num_commas = count_commas(line);
		smush_commas(line);
		line_parse(line);
	}

	/* replace definitions & count orgs */

	for (linenum = 0 ; linenum < a->num_lines ; linenum++)
	{
		line = &a->lines[linenum];
		apply_definitions(line);
		if (line->instruction)
		{
			if (!strcmp(line->instruction->name, "def"))
				parse_definition(line);
			if (!strcmp(line->instruction->name, "org"))
				a->num_orgs++;
			if (line->line_label.name[0] && line->line_label.is_lookup)
				a->num_orgs++;
		}
	}
	a->orgs = (org_t *) malloc(a->num_orgs * sizeof(org_t));

	/* generate code & make note of label offsets, but don't replace labels yet */

	generate_code(a);

	/* build label pointer table */

	build_label_pointer_table(a);

	/* go through code and replace labels */

	replace_labels(a);

	/* go through code and replace lookup table references */

//	replace_lookups(a);

	return error;

}

asm_t *asm_create(char *source)
{
	asm_t *ret;

	ret = (asm_t *) calloc(1, sizeof(asm_t));

	ret->source = source;

	return ret;

}

void line_destroy(line_t *line)
{
}

void asm_destroy(asm_t *a)
{

	int i;

	for (i = 0 ; i < a->num_lines ; i++)
		line_destroy(&a->lines[i]);

}

void dump_str(int indent, char *header, char *str)
{
	int newline;
	char *hdr;
	char *s;

	hdr = malloc(indent+1);

	sprintf(hdr, "%s: [", header);
	printf ("%*s", indent, hdr);

	newline = 0;
	s = str;

	while (*s && !newline)
	{

		if (*s == '\n')
			newline++;

		if (*s == '\t') {
			putchar('\\');
			putchar('t');
		} else if (*s == '\n') {
			putchar('\\');
			putchar('n');
		} else if (*s == '\r') {
			putchar('\\');
			putchar('r');
		} else
			putchar(*s);

		s++;

	}

	printf("]\n");

	free (hdr);

}

void dump_instruction(int indent, char *header, instruction_t *inst)
{
	char *hdr;

	hdr = malloc(indent+1);

	sprintf(hdr, "%s: ", header);
	printf ("%*s", indent-1, hdr);

	printf("%s\n", inst->name);

	free(hdr);

}

void line_dump(int indent_width, line_t *line)
{

	int i;

	dump_str(indent_width, "orig", line->orig_source);
	dump_str(indent_width, "str", line->str);
	printf("%*s", indent_width-1, "commas: ");
	printf("%d\n", line->num_commas);
//	if (line->comment)
//		dump_str(indent_width, "comment", line->comment);
//	dump_str(indent_width, "op1 label", line->operand_labels[0]);
//	dump_str(indent_width, "op2 label", line->operand_labels[1]);
//	dump_str(indent_width, "op3 label", line->operand_labels[2]);

	if (line->line_label.name[0])
	{
		printf("%*s", indent_width-1, "label: ");
		printf("%s at offset %d\n", line->line_label.name, line->line_label.code_pos);
	}
	if (line->instruction)
	{
		dump_instruction(indent_width, "inst", line->instruction);
		printf("%*s", indent_width-1, "num ops: ");
		printf("%d\n", line->num_operands);
		for (i=0 ; i<line->num_operands ; i++)
		{
			if (line->operand_labels[i].name[0])
			{
				printf("%*s", indent_width+2, "op label: ");
				printf("%s at offset %d\n", line->operand_labels[i].name, line->operand_labels[i].code_pos);
			}
			dump_str(indent_width+2, "op", line->operands[i].ident.str);
		}
	}
//	for (i=0 ; i<line->num_commas ; i++)
//		dump_str(indent_width+2, "comma: ", line->commas[i]);

}

void asm_dump_structure(asm_t *a)
{

	int i;

	printf("num_defs: %d\n", a->num_defs);
	for (i=0 ; i<a->num_defs ; i++)
		printf("  [%s] -> [%s]\n", a->defs[i].name, a->defs[i].value);

	printf("\nlines: %d\n", a->num_lines);
	for (i=0 ; i<a->num_lines ; i++)
	{
		printf("\nline %d:\n", i+1);
		line_dump(11, &a->lines[i]);
	}

}

bitqueue_t *asm_to_pinky(char *name, char *source, int flags)
{
	asm_t *a;
	bitqueue_t *ret;
	int rc;

	asm_current = a = asm_create(source);
	a->name = name;

	rc = asm_assemble(a);
	if (rc)
		asm_error(0, 0, "Assembly failed.\n");

	if (flags & ASM_FLAG_DUMP_STRUCTURE)
		asm_dump_structure(a);

	ret = a->code;
	asm_destroy(a);

	return ret;

}


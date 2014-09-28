#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "number.h"
#include "bitqueue.h"
#include "memory.h"
#include "pinky.h"
#include "farm.h"
#include "console.h"

int is_delimeter(char c)
{
	char delimeters[] = { ' ', '\n', '\t', ',', ':', '(', ')' };
	int num_delimeters = sizeof(delimeters);
	int i;

	for (i=0 ; i<num_delimeters ; i++)
		if (c == delimeters[i]) break;

	return i < num_delimeters;
}

void console_process_buffer(console_t *c)
{
	char *str;
	enum { in_arg, between_args } state;

	c->num_args = 0;
	state = is_delimeter(c->buffer[0]) ? between_args : in_arg;

	// Separate line into arguments for further parsing

	str = &c->buffer[0];
	c->arg_text[0] = str;

	while (*str)
	{
		if (state == between_args && !is_delimeter(*str))
		{
			state = in_arg;
			c->arg_text[c->num_args] = str;
		}
		else if (state == in_arg && is_delimeter(*str))
		{
			state = between_args;
			c->arg_len[c->num_args] =
				str - c->arg_text[c->num_args];
			c->num_args++;
		}
		str++;
	}

	// in case the string ends w/o a newline

	if (state == in_arg)
	{
		c->arg_len[c->num_args] =
			str - c->arg_text[c->num_args];
		c->num_args++;
	}

}

void console_command_pinky(console_t *c)
{
	number_t num;
	int pnum;

	// set the current pinky

	number_parse(&num, 16, c->arg_text[1]);
	pnum = num.value;

	sprintf(c->prompt, "pinky #%d: ", pnum);

	c->current_pinky = pnum;
}

void console_command_rom_start(console_t *c)
{
	pinky_t *p;

	p = c->farm->pinky[c->current_pinky];

	if (!p->rom)
		p->rom = bitqueue_create(2048);

	c->rom_entry = c->farm->pinky[c->current_pinky]->rom;

	printf("  rom recording started.\n");
}

void console_command_rom_end(console_t *c)
{
	c->rom_entry = 0;
	printf("  rom recording ended.\n");
}

void console_command_rom_save(console_t *c)
{
	int ret;
	pinky_t *p;
	char *filename;

	filename = c->arg_text[2];
	c->arg_text[2][c->arg_len[2]] = 0;	// need to strip the \n

	p = c->farm->pinky[c->current_pinky];

	ret = bitqueue_save(p->rom, filename);
	if (ret >= 0)
		printf("  %d bits saved to file %s.\n", p->rom->head - p->rom->tail, filename);
}

void console_command_rom_load(console_t *c)
{
	pinky_t *p;
	char *filename;

	filename = c->arg_text[2];
	c->arg_text[2][c->arg_len[2]] = 0;	// need to strip the \n

	p = c->farm->pinky[c->current_pinky];

	if (p->rom)
	{
		if (p->rom->bits)
			free (p->rom->bits);
		free (p->rom);
	}

	p->rom = bitqueue_load(filename);

	if (p->rom)
		printf("  %d bits loaded.\n", p->rom->head);

}

void console_command_reset(console_t *c)
{
	pinky_reset(c->farm->pinky[c->current_pinky]);
}

void console_command_mov(console_t *c)
{
	bitqueue_t *dest;
	number_t addr, data, bits;
	int num_bits;
	pinky_t *p;
	int i;

	p = c->farm->pinky[c->current_pinky];

	if (c->rom_entry)
		dest = c->rom_entry;
	else
		dest = bitqueue_create(2048);

	bitqueue_put_bit(dest, PINKY_INSTRUCTION_MOV >> 1);
	bitqueue_put_bit(dest, PINKY_INSTRUCTION_MOV & 1);
	num_bits = number_parse(&addr, 16, c->arg_text[1]);
	for (i = num_bits - 1 ; i >= 0 ; i--)
		bitqueue_put_bit(dest, addr.value >> i);
	num_bits = number_parse(&data, num_bits, c->arg_text[2]);
	for (i = num_bits - 1 ; i >= 0 ; i--)
		bitqueue_put_bit(dest, data.value >> i);
	num_bits = number_parse(&bits, num_bits, c->arg_text[3]);
	for (i = num_bits - 1 ; i >= 0 ; i--)
		bitqueue_put_bit(dest, bits.value >> i);

	if (!c->rom_entry)
	{
		p->state = PINKY_STATE_GET_INSTRUCTION_BIT0;
		for (i = dest->tail ; i < dest->head ; i++)
			pinky_eat_bit(p, bitqueue_peek_bit(dest, i));
		free (dest->bits);
		free (dest);
	}

}

void console_command_set_n(console_t *c)
{
	bitqueue_t *dest;
	number_t n;
	int num_bits;
	pinky_t *p;
	int i;
	int N;

	p = c->farm->pinky[c->current_pinky];

	num_bits = number_parse(&n, 6, c->arg_text[1]);
	N = n.value - 1;	// because you'd never do 0 bit operands, instruction adds one to the encoding to determine N

	if (c->rom_entry)
		dest = c->rom_entry;
	else
		dest = bitqueue_create(2048);

	bitqueue_put_bit(dest, PINKY_INSTRUCTION_SET_N >> 1);
	bitqueue_put_bit(dest, PINKY_INSTRUCTION_SET_N & 1);
	for (i = num_bits - 1 ; i >= 0 ; i--)
		bitqueue_put_bit(dest, N >> i);

	if (!c->rom_entry)
	{
		p->state = PINKY_STATE_GET_INSTRUCTION_BIT0;
		for (i = dest->tail ; i < dest->head ; i++)
			pinky_eat_bit(p, bitqueue_peek_bit(dest, i));
		free (dest->bits);
		free (dest);
	}

}

void console_command_arm(console_t *c)
{
	bitqueue_t *dest;
	number_t trigger_addr, exec_addr;
	pinky_t *p;
	int i;

	p = c->farm->pinky[c->current_pinky];

	if (c->rom_entry)
		dest = c->rom_entry;
	else
		dest = bitqueue_create(2048);

	bitqueue_put_bit(dest, PINKY_INSTRUCTION_ARM >> 1);
	bitqueue_put_bit(dest, PINKY_INSTRUCTION_ARM & 1);
	number_parse(&trigger_addr, p->N, c->arg_text[1]);
	for (i = p->N - 1 ; i >= 0 ; i--)
		bitqueue_put_bit(dest, trigger_addr.value >> i);
	number_parse(&exec_addr, p->N, c->arg_text[2]);
	for (i = p->N - 1 ; i >= 0 ; i--)
		bitqueue_put_bit(dest, exec_addr.value >> i);

	if (!c->rom_entry)
	{
		p->state = PINKY_STATE_GET_INSTRUCTION_BIT0;
		for (i = dest->tail ; i < dest->head ; i++)
			pinky_eat_bit(p, bitqueue_peek_bit(dest, i));
		free (dest->bits);
		free (dest);
	}

}

void console_command_wait(console_t *c)
{
	bitqueue_t *dest;
	pinky_t *p;
	int i;

	p = c->farm->pinky[c->current_pinky];

	if (c->rom_entry)
		dest = c->rom_entry;
	else
		dest = bitqueue_create(2048);

	bitqueue_put_bit(dest, PINKY_INSTRUCTION_WAIT >> 1);
	bitqueue_put_bit(dest, PINKY_INSTRUCTION_WAIT & 1);

	if (!c->rom_entry)
	{
		p->state = PINKY_STATE_GET_INSTRUCTION_BIT0;
		for (i = dest->tail ; i < dest->head ; i++)
			pinky_eat_bit(p, bitqueue_peek_bit(dest, i));
		free (dest->bits);
		free (dest);
	}

}

void console_command_help(console_t *c)
{
	printf("  pinky <number>: attach to specified pinky.\n");
	printf("  verbose <\"on\"|\"off\">: on by default, print pinky state after each command.\n");
	printf("  reset: reset the attached pinky.\n");
	printf("  rom <\"start\"|\"end\">: enter following sequence of commands into the boot rom of the attached pinky.\n");
	printf("  rom save <filename>: save boot rom to file.\n");
	printf("  rom load <filename>: load boot rom from a file.\n");
	printf("  mov <dest>, <src>, <len>: instruction to mov <len> bits from <src> to <dest>\n");
	printf("  arm <trigger>, <execution>: instruction to execute code at <execution> address whenever <trigger> is written.\n");
	printf("  set_n <number>: instruciton to set instruction operand width.  <number> can be 1 to 64.\n");
	printf("  wait: instruciton to put pinky in wait state.\n");
	printf("  help: you're looking at it.\n");
	printf("  quit: exit program.\n");
}

int is_match(console_t *c, int i, char *str)
{
	return !strncasecmp(c->arg_text[i], str, c->arg_len[i]);
}

void console_display(console_t *c, char *str)
{
	printf("%s", str);
	fflush(stdin);
}

int console_execute_command(console_t *c)
{

	int end;

	end = 0;

	if (is_match(c, 0, "pinky"))
	{
		console_command_pinky(c);
	}
	else if (is_match(c, 0, "reset"))
	{
		console_command_reset(c);
	}
	else if (is_match(c, 0, "verbose"))
	{
		if (is_match(c, 1, "on"))
			c->farm->pinky[c->current_pinky]->verbose = 1;
		else if (is_match(c, 1, "off"))
			c->farm->pinky[c->current_pinky]->verbose = 0;
        else
            console_display(c, "error: usage: verbose [on | off]\n");
	}
	else if (is_match(c, 0, "rom"))
	{
		if (is_match(c, 1, "start"))
		{
			console_command_rom_start(c);
		}
		else if (is_match(c, 1, "end"))
		{
			console_command_rom_end(c);
		}
		else if (is_match(c, 1, "save"))
		{
			console_command_rom_save(c);
		}
		else if (is_match(c, 1, "load"))
		{
			console_command_rom_load(c);
		}
		else
		{
			console_display(c, "error: usage: rom [start | end | save <filename> | load <filename>]\n");
		}
	}
	else if (is_match(c, 0, "mov"))
	{
		console_command_mov(c);
	}
	else if (is_match(c, 0, "arm"))
	{
		console_command_arm(c);
	}
	else if (is_match(c, 0, "wait"))
	{
		console_command_wait(c);
	}
	else if (is_match(c, 0, "set_n"))
	{
		console_command_set_n(c);
	}
	else if (is_match(c, 0, "quit"))
	{
		end = 1;
	}
	else if (is_match(c, 0, "help"))
	{
		console_command_help(c);
	}
	else
	{
		console_display(c, "error: unrecognized command\n");
	}

	return end;

}

int console_get_line(console_t *c)
{

	printf("%s", c->prompt);

	if (fgets(c->buffer, sizeof(c->buffer), stdin))
		c->len = strlen(c->buffer);
	else
		c->len = 0;

	return c->len;
}

console_t *console_create(farm_t *f)
{
	console_t *c;

	c = (console_t *) malloc(sizeof(console_t));
	c->verbose = 1;

	c->farm = f;

	printf("%d pinky processors available\n", f->num_pinkies);
	printf("%d triggers per pinky\n", f->pinky[0]->num_triggers);
	printf("%d bits of memory available\n", f->local_address_bits);

	sprintf(c->buffer, "pinky 0\n");
	console_process_buffer(c);
	console_command_pinky(c);

	return c;

}

void console_run(console_t *c)
{
	int end;
	int len;

	end = 0;

	while (!end)
	{
		len = console_get_line(c);
	
		if (len == 0)
			end = 1;
		else
		{
			console_process_buffer(c);
			end = console_execute_command(c);
		}

		farm_quiesce(c->farm);
	}

}

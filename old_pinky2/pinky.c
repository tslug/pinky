#include "number.h"
#include "bitqueue.h"
#include "memory.h"
#include "pinky.h"
#include "farm.h"

#include <stdlib.h>
#include <stdio.h>

void pinky_dump_state(pinky_t *);

void pinky_reset(pinky_t *p)
{
	int bit;
	int i;

  	p->state = PINKY_STATE_GET_INSTRUCTION_BIT0;
	if (p->verbose)
		pinky_dump_state(p);

	if (p->rom)
	{
		i = p->rom->tail;
		while (p->state != PINKY_STATE_WAITING)
		{
			bit = bitqueue_peek_bit(p->rom, i);
			pinky_eat_bit(p, bit);
			i++;
		}
	}

}

pinky_t *pinky_create(struct farm_s *farm, bitqueue_t *rom, memory_t *mem, int num_triggers)
{

	pinky_t *p;

	p = (pinky_t *) malloc(sizeof(pinky_t));
	p->farm = farm;
	p->rom = rom;
	p->mem = mem;
	p->verbose = 0;
	p->triggered = 0;
	p->num_triggers = num_triggers;
	p->trigger = (trigger_t *) calloc(1, num_triggers * sizeof(trigger_t));
	pinky_reset(p);

	return p;

}

void pinky_mov(pinky_t *p)
{
	unsigned long long src, dest, len;
	int i;
	pinky_t *p_src, *p_dest;

	dest = p->operand[0].value;
	src = p->operand[1].value;
	len = p->operand[2].value;

	dest |= p->regs[0] & ~((1 << p->N) - 1);
	src |= p->regs[1] & ~((1 << p->N) - 1);
	len |= p->regs[2] & ~((1 << p->N) - 1);

	p->regs[0] = dest;
	p->regs[1] = src;
	p->regs[2] = len;

	p_src = farm_find_pinky_memory(p->farm, src);
	p_dest = farm_find_pinky_memory(p->farm, dest);

	if (p_src == p && p_dest == p)
		memory_local_move(p->mem, dest, src, len);
	else
		memory_global_move(p_dest->mem, dest, p_src->mem, src, len);

	for (i = 0 ; i < p_dest->num_triggers && dest != p_dest->trigger[i].trigger_addr; i++) ;

	if (i != p_dest->num_triggers)
	{
		p_dest->trigger[i].triggered = 1;
		p_dest->triggered++;
		p_dest->farm->triggered++;
	}

}

void pinky_arm(pinky_t *p)
{
	unsigned long long trigger_addr, exec_addr;
	int i;

	trigger_addr = p->operand[0].value;
	exec_addr = p->operand[1].value;

	for (i = 0 ; i < p->num_triggers && !p->trigger[i].trigger_addr ; i++) ;

	if (i != p->num_triggers)
	{
		p->trigger[i].triggered = 0;
		p->trigger[i].trigger_addr = trigger_addr;
		p->trigger[i].exec_addr = exec_addr;
	}

}

void pinky_execute_trigger(pinky_t *p, int trigger_num)
{
	trigger_t *t;
	int bit;

	t = &p->trigger[trigger_num];

	p->triggered--;
	p->farm->triggered--;

	t->triggered = 0;
	t->trigger_addr = 0;

	p->state = PINKY_STATE_GET_INSTRUCTION_BIT0;
	while (p->state != PINKY_STATE_WAITING)
	{
		bit = memory_read(p->mem, t->exec_addr);
		pinky_eat_bit(p, bit);
		t->exec_addr++;
	}

}

void pinky_quiesce(pinky_t *p)
{
	int i;

	while (p->triggered)
	{
		for (i = 0 ; i < p->num_triggers ; i++)
			if (p->trigger[i].triggered)
				pinky_execute_trigger(p, i);
	}

}

void pinky_display_operand(pinky_t *p, int operand)
{
	number_t temp;
	
	temp.bits = p->N;
	temp.value = p->operand[operand].value;
	number_display(&temp);
}

void pinky_dump_state(pinky_t *p)
{

	int i;

	printf("N=%d, state=%d", p->N, p->state);
	switch (p->state)
	{
		case PINKY_STATE_WAITING:
			printf(" (WAITING)\n");
			break;
		case PINKY_STATE_GET_INSTRUCTION_BIT0:
			printf(" (GET_INSTRUCTION_BIT0)\n");
			if (p->operand_index)
			{
				printf("  last instruction: ");
				if (p->instruction == PINKY_INSTRUCTION_MOV)
				{
					printf("MOV ");
					pinky_display_operand(p, 0);
					printf(", ");
					pinky_display_operand(p, 1);
					printf(", %d\n", (int) p->operand[2].value);
				}
				else if (p->instruction == PINKY_INSTRUCTION_ARM)
				{
					printf("ARM ");
					pinky_display_operand(p, 0);
					printf(", ");
					pinky_display_operand(p, 1);
					printf("\n");
				}
				else if (p->instruction == PINKY_INSTRUCTION_SET_N)
				{
					printf("SET_N %d\n", (int) p->operand[0].value + 1);
				}
				else if (p->instruction == PINKY_INSTRUCTION_WAIT)
				{
					printf("WAIT\n");
				}
			}
			break;
		case PINKY_STATE_GET_INSTRUCTION_BIT1:
			printf(" (GET_INSTRUCTION_BIT1)\n");
			break;
		case PINKY_STATE_GET_OPERAND:
			printf(" (GET_OPERAND)\n");
			printf("  instruction: ");
			switch (p->instruction)
			{
				case PINKY_INSTRUCTION_MOV:
					printf("MOV ");
					break;
				case PINKY_INSTRUCTION_ARM:
					printf("ARM ");
					break;
				case PINKY_INSTRUCTION_WAIT:
					printf("WAIT ");
					break;
				case PINKY_INSTRUCTION_SET_N:
					printf("SET_N ");
					break;
			}
			for (i = 0 ; i < p->operand_index ; i++)
			{
				pinky_display_operand(p, i);
				printf(", ");
			}
			for ( ; i < p->num_operands ; i++)
				printf(", xxx");
			printf("\n");
			break;
	}
}

void pinky_eat_bit(pinky_t *p, int bit)
{

	int dump_worthy;

	bit = bit & 1;
	pinky_state_e orig_state;

	dump_worthy = 0;
	orig_state = p->state;

	if (p->state == PINKY_STATE_GET_INSTRUCTION_BIT0)
	{
		p->instruction = bit << 1;
		p->state = PINKY_STATE_GET_INSTRUCTION_BIT1;
		dump_worthy = 1;
	}
	else if (p->state == PINKY_STATE_GET_INSTRUCTION_BIT1)
	{
		p->instruction |= bit;
		if (p->instruction == PINKY_INSTRUCTION_MOV)
		{
			p->state = PINKY_STATE_GET_OPERAND;
			p->num_operands = 3;
			p->operand_index = 0;
			p->operand[0].value = 0;
			p->operand_bits = p->N;
		}
		else if (p->instruction == PINKY_INSTRUCTION_ARM)
		{
			p->state = PINKY_STATE_GET_OPERAND;
			p->num_operands = 2;
			p->operand_index = 0;
			p->operand[0].value = 0;
			p->operand_bits = p->N;
		}
		else if (p->instruction == PINKY_INSTRUCTION_WAIT)
		{
			p->state = PINKY_STATE_WAITING;
		}
		else if (p->instruction == PINKY_INSTRUCTION_SET_N)
		{
			p->state = PINKY_STATE_GET_OPERAND;
			p->num_operands = 1;
			p->operand_index = 0;
			p->operand[0].value = 0;
			p->operand_bits = 6;
		}
		dump_worthy = 1;
	}
	else if (p->state == PINKY_STATE_GET_OPERAND)
	{
		p->operand_bits--;
		p->operand[p->operand_index].value |= bit << p->operand_bits;
		if (p->operand_bits == 0)
		{
			p->num_operands--;
			p->operand_index++;
			p->operand_bits = p->N;
			if (!p->num_operands)
			{
				if (p->instruction == PINKY_INSTRUCTION_MOV)
					pinky_mov(p);
				else if (p->instruction == PINKY_INSTRUCTION_ARM)
					pinky_arm(p);
				else // PINKY_INSTRUCTION_SET_N
				{
					p->N = p->operand[0].value + 1;
				}
				p->state = PINKY_STATE_GET_INSTRUCTION_BIT0;
			}
			dump_worthy = 1;
		}
	}

	if (p->verbose && dump_worthy)
		pinky_dump_state(p);

}

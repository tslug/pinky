#include <stdlib.h>

#include "number.h"
#include "bitqueue.h"
#include "memory.h"
#include "pinky.h"
#include "farm.h"

farm_t *farm_create(int num_pinkies, int num_triggers, int addr_bits, unsigned long long farm_addr, bitqueue_t *rom)
{

	int i;
	farm_t *f;
	memory_t *mem;

	f = (farm_t *) malloc(sizeof(farm_t));

	f->triggered = 0;
	f->local_address_bits = addr_bits;
	f->farm_address = farm_addr;

	f->num_pinkies = num_pinkies;
	f->pinky = (pinky_t **) malloc(num_pinkies * sizeof(pinky_t*));
	for (i = 0 ; i < num_pinkies ; i++)
	{
		mem = memory_create(1 << addr_bits);
		f->pinky[i] = pinky_create(f, rom, mem, num_triggers);
	}

	return f;

}

void farm_quiesce(farm_t *f)
{

	int i;

	while (f->triggered)
	{
		for (i = 0 ; i < f->num_pinkies ; i++)
			pinky_quiesce(f->pinky[i]);
	}

}

pinky_t *farm_find_pinky_memory(farm_t *f, unsigned long long addr)
{
	pinky_t *pinky;
	int	pinky_num;

	if (addr >= f->farm_address && addr < f->farm_address + f->num_pinkies * f->farm_address)
	{
		pinky_num = (addr - f->farm_address) >> f->local_address_bits;
		pinky = f->pinky[pinky_num];
	}
	else
		pinky = 0;

	return pinky;

}


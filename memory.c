#include <stdlib.h>

#include "bitqueue.h"
#include "memory.h"

/*
unsigned char pos_mask[8] = { 0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe };
int pos_shift[8] = { 7, 6, 5, 4, 3, 2, 1, 0 };
unsigned char shifted_bit[2][8] = 
{
	{
		0, 0, 0, 0, 0, 0, 0, 0
	},
	{
		0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
	}
};
*/

memory_t *memory_create(int num_bits)
{
	int bytes;
	memory_t *m;

	m = (memory_t *) malloc(sizeof (memory_t));

	m->num_bits = num_bits;
	bytes = (num_bits+7)/8;

	m->bits = (unsigned char *) malloc(bytes);

	return m;
}

void memory_local_move(memory_t *mem, unsigned long long dest, unsigned long long src, unsigned long long bits)
{

	int bit;
	unsigned long long index;

	while (bits)
	{
		index = src >> 3;
		bit = (mem->bits[index] >> pos_shift[src & 7]) & 1;

		index = dest >> 3;
		mem->bits[index] &= pos_mask[dest & 7];
		mem->bits[index] |= shifted_bit[bit][dest & 7];

		src++;
		dest++;
		bits--;
	}

}

void memory_global_move(memory_t *dest_mem, unsigned long long dest, memory_t *src_mem, unsigned long long src, unsigned long long bits)
{

	int bit;
	unsigned long long index;

	while (bits)
	{
		index = src >> 3;
		bit = (src_mem->bits[index] >> pos_shift[src & 7]) & 1;

		index = dest >> 3;
		dest_mem->bits[index] &= pos_mask[dest & 7];
		dest_mem->bits[index] |= shifted_bit[bit][dest & 7];

		src++;
		dest++;
		bits--;
	}

}

int memory_read(memory_t *mem, unsigned long long addr)
{
	unsigned long long index;
	int bit;

	index = addr >> 3;
	bit = (mem->bits[index] >> pos_shift[addr & 7]) & 1;

	return bit;
}


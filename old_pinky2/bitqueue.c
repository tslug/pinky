#include <stdlib.h>
#include <stdio.h>

#include "bitqueue.h"

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

int bits_to_bytes(int bits)
{
	return (bits + 7) / 8;
}

bitqueue_t *bitqueue_create(int num_bits)
{
	bitqueue_t *b;

	b = (bitqueue_t *) malloc(sizeof(bitqueue_t));
	b->head = 0;
	b->tail = 0;
	b->num_bits = num_bits;
	if (num_bits)
		b->bits = (unsigned char *) malloc(bits_to_bytes(num_bits));

	return b;

}

void bitqueue_destroy(bitqueue_t *b)
{
	free(b->bits);
	free(b);
}

bitqueue_t *bitqueue_load(char *filename)
{
	bitqueue_t *b;
	char buffer[256];
	FILE *f;
	int num_bits, len;
	int read_size;
	int i;

	b = 0;
	f = fopen(filename, "r");

	if (f)
	{

		fgets(buffer, sizeof(buffer), f);
		sscanf(buffer, "num_bits: %d, len: %d", &num_bits, &len);

		b = bitqueue_create(num_bits);

		while (len)
		{
			if (len > 72)
				read_size = 72;
			else
				read_size = len;

			fgets(buffer, sizeof(buffer), f);
			for (i = 0 ; i < read_size ; i++)
				bitqueue_put_bit(b, buffer[i] - '0');

			len -= read_size;
		}

		fclose(f);

	}
	else
		printf("  file %s not found.\n", filename);

	return b;

}

int bitqueue_save(bitqueue_t *b, char *filename)
{
	char buffer[256];
	FILE *f;
	int len;
	int write_size;
	int i;
	int tail;
	int ret;

	ret = -1;

	f = stdin;
	if (filename)
		f = fopen(filename, "w");

	if (f)
	{

		len = b->head - b->tail;
		fprintf(f, "num_bits: %d, len: %d\n", b->num_bits, len);

		tail = b->tail;
		while (len)
		{
			if (len > 72)
				write_size = 72;
			else
				write_size = len;

			for (i = 0 ; i < write_size ; i++)
				buffer[i] = '0' + bitqueue_peek_bit(b, tail + i);

			buffer[i] = '\n';
			buffer[i+1] = 0;

			fputs(buffer, f);

			len -= write_size;
			tail += write_size;
		}

		ret = 0;
		if (f != stdin)
			ret = fclose(f);

	}
	else
		printf("  file %s could not be opened.\n", filename);

	return ret;

}

void bitqueue_fix_tail_after_realloc(bitqueue_t *b)
{
	int i, bit, index;

	for (i = b->tail ; i < b->num_bits - 2048 ; i++)
	{
		index = i >> 3;
		bit = (b->bits[index] >> pos_shift[i & 7]) & 1;

		index = (i+2048) >> 3;
		b->bits[index] &= pos_mask[(i+2048) & 7];
		b->bits[index] |= shifted_bit[bit][(i+2048) & 7];
	}
}

void bitqueue_put_bit(bitqueue_t *b, int bit)
{
	int index;

	bit = bit & 1;

	index = b->head >> 3;
	b->bits[index] &= pos_mask[b->head & 7];
	b->bits[index] |= shifted_bit[bit][b->head & 7];

	b->head++;

	if (b->head == b->tail)
	{
		b->bits = (unsigned char *) realloc(b->bits, b->num_bits + 2048/8);
		b->num_bits += 2048;
		bitqueue_fix_tail_after_realloc(b);
	}

}

int bitqueue_get_bit(bitqueue_t *b)
{
	int index;
	int bit;

	index = b->tail >> 3;
	bit = (b->bits[index] >> pos_shift[b->tail & 7]) & 1;
	b->tail++;

	return bit;
}

void bitqueue_dump(bitqueue_t *b)
{
	int i, index, bit;

	printf("  %d bits: ", b->head - b->tail);

	for (i = b->tail ; i < b->head ; i++)
	{
		index = i >> 3;
		bit = (b->bits[index] >> pos_shift[i & 7]) & 1;
		printf("%c", '0' + bit);
	}

	printf("\n");
}

int bitqueue_peek_bit(bitqueue_t *b, int i)
{
	int index;
	int bit;

	index = i >> 3;
	bit = (b->bits[index] >> pos_shift[i & 7]) & 1;

	return bit;
}


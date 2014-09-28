#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "number.h"

int is_binary(char n)
{
	return n >= '0' && n <= '1';
}

int is_octal(char n)
{
	return n >= '0' && n <= '7';
}

int is_decimal(char n)
{
	return n >= '0' && n <= '9';
}

int is_hex(char n)
{
	return is_decimal(n) || (tolower(n) >= 'a' && tolower(n) <= 'f');
}

void number_display(number_t *n)
{
	int i;

	for (i = 1 ; i <= n->bits ; i++)
		printf("%c", '0' + (((int) n->value >> (n->bits - i)) & 1));
	printf("b");
}

int number_parse(number_t *n, int default_bits, char *str)
{

	n->bits = 0;
	n->value = 0;

	// binary numbers start with a '#'
	if (*str == '#')
	{
		str++;
		while (is_binary(*str))
		{
			n->value = (n->value << 1) + digittoint(*str);
			n->bits = n->bits + 1;
			str++;
		}
	}

	// hex, octal, and 0 start with '0'
	else if (*str == '0')
	{
		str++;
		if (*str == 'x') // hex numbers start with "0x"
		{
			str++;
			while (is_hex(*str))
			{
				n->value = (n->value << 4) + digittoint(*str);
				n->bits = n->bits + 4;
				str++;
			}

		}

		else if (is_octal(*str)) // octal numbers start with just a '0'
		{
			while (is_octal(*str))
			{
				n->value = (n->value << 3) + digittoint(*str);
				n->bits = n->bits + 3;
				str++;
			}
		}

		else // just plain 0, so assume # of bits same as last data entry size
		{
			n->value = 0;
			n->bits = default_bits;
		}

	}

	// decimal, assume # of bits same as last data entry size
	else if (is_decimal(*str))
	{
		while (is_decimal(*str))
		{
			n->value = (n->value * 10) + digittoint(*str);
			str++;
		}
		n->bits = default_bits;
	}

	return n->bits;

}


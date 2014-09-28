#include <stdio.h>

main()
{
	int len;
	FILE *f;
	char buffer[256];

	f = stdin;

	while (len = fread(buffer, 1, 256, f))
	{
		printf("len=%d, buf=[%s]\n", len, buffer);
	}

}

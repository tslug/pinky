#include "bitqueue.h"
#include "number.h"
#include "memory.h"
#include "pinky.h"
#include "farm.h"
#include "console.h"
#include "asm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void usage(void)
{
	printf("usage:    p asm [-o out_file] [in_file]\n");
	printf("          p sim [num_pinkies] [num_triggers] [address_bits] [rom]\n\n");
	printf("p asm [-o out_file] [in_file]\n\n");
	printf("          out_file:		Output file of compiled asm.\n");
	printf("          in_file:		Asm source file.\n\n");
	printf("p sim [num_pinkies] [num_triggers] [address_bits] [rom]\n\n");

	printf("          num_pinkies:          Number of Pinky processors.  Default=64.\n");
	printf("          num_triggers:         Number of triggers per Pinky.  Default=8.\n");
	printf("          address_bits:         Local Pinky memory size is 2^address_bits.  Default=16.\n");
	printf("          rom:                  Filename of boot ROM image for Pinkies.  Default=none.\n\n");

	printf("examples: p sim 64           - Creates 64 Pinkies.\n");
	printf("          p sim 32 16        - Creates 32 Pinkies with 16 triggers each.\n");
	printf("          p sim 1024 32 32   - creates 1024 Pinkies, 32 triggers each, 4Gbits memory each.\n\n");
}

int sim_main(int argc, char **argv)
{
	int num_pinkies;
	int num_triggers;
	int memory;
	bitqueue_t *rom;
	console_t *c;
	farm_t *farm;

	num_pinkies = 64;
	num_triggers = 8;
	memory = 16;
	rom = 0;

	if (argc == 1)
		num_pinkies = atoi(argv[0]);
	else if (argc == 2)
		num_triggers = atoi(argv[1]);
	else if (argc == 3)
		memory = atoi(argv[2]);
	else if (argc == 4)
		rom = bitqueue_load(argv[3]);

	farm = farm_create(num_pinkies, num_triggers, memory, 1 << memory, rom);

	c = console_create(farm);

	console_run(c);

	return 0;

}

int asm_main(int argc, char **argv)
{

	int len, ret, rc;
	FILE *in;
	char *source;
	char *in_filename;
	char *out_filename;
	bitqueue_t *pinky_machine_code;

	in = stdin;
	in_filename = 0;
	out_filename = 0;

	ret = 0;

	if (argc == 1)
		in_filename = argv[0];
	if (argc == 2 || argc == 3)
		out_filename = argv[1];
	if (argc == 3)
		in_filename = argv[2];

	// load input file

	if (in_filename)
		in = fopen(in_filename, "r");

	len = 0;
	source = (char *) malloc(256);

    do {
        rc = fread(source+len, 1, 256, in);
        len += rc;
        if (rc == 256)
            source = (char *) realloc(source, len+256);
    } while (rc == 256);

	if (in != stdin)
		fclose(in);

	// assemble the asm

	pinky_machine_code = asm_to_pinky(in_filename, source, ASM_FLAG_DUMP_STRUCTURE);
	if (!pinky_machine_code)
		ret = -1;

	// write out the output file

//	ret = bitqueue_save(pinky_machine_code, out_filename);
//	bitqueue_destroy(pinky_machine_code);

	free(source);

	return ret;

}

int main(int argc, char **argv)
{

	if (argc == 2 && !strcmp(argv[1], "--help"))
	{
		usage();
		return 0;
	}

	if (argc < 2)
	{
		usage();
		return -1;
	}

	if (!strcmp(argv[1], "sim"))
		return sim_main(argc-2, &argv[2]);

	else if (!strcmp(argv[1], "asm"))
		return asm_main(argc-2, &argv[2]);

	return 0;

}

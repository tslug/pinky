/*  Pinky assembler
    Copyright (C) 1998  Ronnie Misra rgmisra@mit.edu

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>

#define BIT(b) (c & (1 << b)) >> b

unsigned char c = 0;
char bl = 7; /* bits left in c (minus 1) */
char start = 7;
int flushflag = 0;

void bp() {
  int j;

  if (flushflag) {
    for (j = start; j >= 0; j--)  /* print bits */
      printf ("%d", BIT(j));
    start = 7;
  }
  else
    printf("%c", c); /* print char value */
}

void flush(char s) {
  int j;
  for (j = start; j > bl; j--)
    printf ("%d", BIT(j));
  printf ("%c", s);
  start = bl;
}

int fl = 1;

void bprinti(int i, int bits) { /* binary print int */
  int j;
  for (j = bits - 1; j >= 0; j--) {
    if (i & (1 << j))
      c |= (1 << bl);
    bl--;
    if (bl == -1) {
      bp();
      c = 0;
      bl = 7;
    }
  }
  if (flushflag && fl)
    flush(' ');
}

void bprintll(long long l, int bits) { /* binary print long long */
  if (bits > 32) {
    fl = 0;
    bprinti(l >> 32, bits - 32);
    bprinti(l & 0xFFFFFFFF, 32);
    fl = 1;
    if (flushflag)
      flush(' ');
  }
  else
    bprinti(l & 0xFFFFFFFF, bits);
}

void reada(char *s, void *p, char *i, char *a) {
                      /* read an argument, or return 0 if arg is missing */
  char foo[20];
  if (feof(stdin)) {
    fprintf (stderr, "missing arg %s for instruction %s\n",
	    a, i);
    bprinti(0, 8);
    exit(-1);
  }
  scanf("%s", foo);
  sscanf(foo, s, p);
}

main(int argc, char *argv[]) {
  char op[5], arg;
  unsigned long long addr;
  unsigned int n;

  if (argc > 1 && !strcmp(argv[1], "-f"))
    flushflag = 1;

  while(!feof(stdin) && scanf("%s", op) > 0) {
    switch (op[0]) {
    case 'm':             /* move */
      bprinti(0, 2);        /* instruction 00 */

      reada("%d",  &n,    "mov", "n");
      bprinti(n, 6);                       /* arg n (6 bits) */
      reada("%Ld", &addr, "mov", "source_addr");
      bprintll(addr, n);                   /* arg source_addr (n bits) */
      reada("%Ld", &addr, "mov", "dest_addr");
      bprintll(addr, n);                   /* arg dest_addr (n bits) */
      reada("%Ld", &addr, "mov", "len");
      bprintll(addr, n);                   /* arg len (n bits) */

      break;
    case 'a':             /* arm */
      bprinti(2, 2);        /* instruction 10 */

      reada("%d",  &n,    "arm", "n");
      bprinti(n, 6);                       /* arg n (6 bits) */
      reada("%Ld", &addr, "arm", "trigger_addr");
      bprintll(addr, n);                   /* arg trigger_addr (n bits) */
      reada("%Ld", &addr, "arm", "action_addr");
      bprintll(addr, n);                   /* arg action_addr (n bits) */

      break;
    case 'w':            /* wait */
      bprinti(3, 2);        /* instruction 11 */
      break;
    default:
      fprintf(stderr, "bad instruction %s\n", op);
      bprinti(0, 8);
      exit(-1);
    }
    if (flushflag)
      flush('\n');
  }
  if (bl != 7)
    bprinti(0, 8);
}

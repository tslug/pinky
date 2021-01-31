/*  Pinky CPU simulator
    Copyright (C) 1998  James Roberton jsrobert@mit.edu

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
#include <stdlib.h>

#define MV 0
#define ARM 2
#define WAIT 3

typedef unsigned long long paddr;
typedef unsigned int uint32;
typedef unsigned char uint8;

struct pinky {
    size_t numcpus;
    struct pcpu *proc;
};

struct pcpu {
    paddr ip;
    paddr op1mask;
    paddr op2mask;
    paddr op3mask;
    int run; /* 0 for wait, 1 for go */
};

struct trigger {
    paddr addr, action;
    struct trigger *next;
};

struct queue {
    void *ptr;
    struct queue *next;
};

#define fatal(msg) do { printf(msg); exit(EXIT_FAILURE); } while(0);

uint8 readbyte(paddr addr, uint8 *memory);
paddr readpaddr(paddr addr, uint8 *memory);
void writepaddr(paddr src, paddr dest, paddr len, uint8 *memory);
void p_run(struct pinky *comp, uint8 *memory, struct trigger *trigs, struct trigger *trigq);

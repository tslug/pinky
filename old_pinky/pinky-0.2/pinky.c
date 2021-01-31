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
   
#include "pinky.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

uint8 readbyte(paddr addr, uint8 *memory) {
    int bigaddr, subaddr;
    uint8 tmp;
    
    bigaddr = addr >> 3;
    subaddr = addr & 7;

    tmp = (memory[bigaddr] << subaddr) & (((1 << (8 - subaddr)) - 1) << subaddr);
    tmp |= (memory[bigaddr + 1] >> (8 - subaddr)) & ((1 << subaddr) - 1);

    return tmp;
}

/* unoptimized readpaddr */
paddr readpaddr(paddr addr, uint8 *memory) {
    paddr tmp;

    tmp = (paddr)readbyte(addr, memory) << (paddr)28;
    tmp |= (paddr)readbyte(addr+8, memory) << (paddr)20;
    tmp |= (paddr)readbyte(addr+16, memory) << (paddr)12;
    tmp |= (paddr)readbyte(addr+24, memory) << (paddr)4;
    tmp |= (paddr)readbyte(addr+28, memory) & (paddr)15;

    return tmp;
}

void writepaddr(paddr src, paddr dest, paddr len, uint8 *memory) {
    paddr destbigaddr = dest >> 3;
    paddr destsubaddr = dest & 7;
    paddr lenbigaddr = len >> 3;
    paddr lensubaddr = len & 7;
    uint8 tmp, tmp2, tmpread;
    paddr i;

    if(len == 0)
	return;

    tmp = readbyte(destbigaddr << 3, memory);
    tmp2 = readbyte((destbigaddr + 1) << 3, memory);

    for(i = 0; i < lenbigaddr; i++) {
	tmp2 = readbyte((destbigaddr + i + 1) << 3, memory);
	/* mask data in */
	tmpread = readbyte(src + (i << 3), memory);
	tmp = (tmp & ~((1 << (8 - destsubaddr)) - 1)) | (tmpread >> destsubaddr);
	tmp2 = (tmpread << (8 - destsubaddr)) | (tmp & (8 - destsubaddr));

	memory[destbigaddr + i] = tmp;
	tmp = tmp2;
    }

    /* cleanup tail */
    if(destsubaddr + lensubaddr > 8) {
	/* hard case */
	tmpread = readbyte(src + (i << 3), memory);
	memory[destbigaddr + i] = ((tmp2 & ~((1 << (8 - destsubaddr)) - 1)) | 
				   (tmpread >> destsubaddr));
	tmpread = readbyte(src + ((i + 1) << 3), memory);
	tmp2 = readbyte((destbigaddr + i + 1) << 3, memory);
	memory[destbigaddr + i + 1] = ((tmpread << (16 - destsubaddr - lensubaddr)) | 
				       (tmp2 & (16 - destsubaddr - lensubaddr)));
    } else {
	tmpread = readbyte(src + (i << 3), memory);
	memory[destbigaddr + i] = ((tmp2 & (uint8)~((1 << (8 - destsubaddr)) - 1)) | 
				   ((tmpread & (uint8)~((1 << (8 - lensubaddr)) - 1)) >> destsubaddr) |
				   (tmp2 & (uint8)((1 << (8 - lensubaddr - destsubaddr)) - 1)));
    }
}

void p_run(struct pinky *comp, uint8 *memory, struct trigger *trigs, struct trigger *trigq) {
    size_t i, j;
    uint8 tmp;
    int N, nofoundcpu;
    struct trigger *last = trigs, *cur, *fixup;
    struct trigger *lasttq = trigq;
    struct trigger *ptrtmp;
    paddr dest;

    while(1) {
	for(i = 0; i < comp->numcpus; i++) {
	    if((comp->proc[i].run)) {
		tmp = readbyte(comp->proc[i].ip, memory);
		switch(tmp >> 6) {
		case MV:
		    N = tmp & 63;
		    if(N > 36)
			fatal("bogus N value\n");
		    if(N == 36) {
			comp->proc[i].op1mask = readpaddr(comp->proc[i].ip + 8, memory);
			comp->proc[i].op2mask = readpaddr(comp->proc[i].ip + 44, memory);
			comp->proc[i].op3mask = readpaddr(comp->proc[i].ip + 80, memory);
		    }
		    dest = ((readpaddr(comp->proc[i].ip + 8 + N, memory) >> (36 - N)) |
			    (comp->proc[i].op2mask & ~((1 << N) - 1)));
		    writepaddr( ((readpaddr(comp->proc[i].ip + 8, memory) >> (36 - N)) |
				 (comp->proc[i].op1mask & ~((1 << N) - 1))),
				dest,
				((readpaddr(comp->proc[i].ip + 8 + (2 * N), memory) >> (36 - N)) |
				 (comp->proc[i].op3mask & ~((1 << N) - 1))),
				memory);
		    /* bad bad bad, should get rid of linear scans */
		    for(cur = trigs, fixup = trigs; cur->next != NULL; cur = cur->next) {
			nofoundcpu = 1;
			if(cur->addr == dest) {
			    for(j = 0; j < comp->numcpus; j++)
				if(!comp->proc[j].run) {
				    comp->proc[j].ip = cur->action;
				    comp->proc[j].run = 1;
				    /* inherit operand masks? */
				    nofoundcpu = 0;
				    break; /* try to break to trigs loop */
				}
			    if(nofoundcpu) { /* add this trigger to queue */
				lasttq->addr = cur->addr;
				lasttq->action = cur->action;
				lasttq->next = (struct trigger *)calloc(1, sizeof(struct trigger));
				if(lasttq->next == NULL)
				    fatal("bad arm calloc\n");
				lasttq = lasttq->next;
			    }
			    if(cur == trigs) { /* start of list */
				ptrtmp = trigs->next;
				free(trigs);
				trigs = ptrtmp;
			    } else {
				ptrtmp = cur->next;
				free(fixup->next);
				fixup->next = ptrtmp;
			    }
			    cur = ptrtmp;
			    if(cur->next == NULL)
				break;
			}
			fixup = cur;
		    }
		    comp->proc[i].ip += 8 + (3 * N);
		    break;
		case ARM:
		    N = tmp & 63;
		    if(N > 36)
			fatal("bogus N value\n");
		    if(N == 36) {
			comp->proc[i].op1mask = readpaddr(comp->proc[i].ip + 8, memory);
			comp->proc[i].op2mask = readpaddr(comp->proc[i].ip + 44, memory);
		    }
		    last->addr = ((readpaddr(comp->proc[i].ip + 8, memory) >> (36 - N)) |
				  (comp->proc[i].op1mask & ~((1 << N) - 1)));
		    last->action = ((readpaddr(comp->proc[i].ip + 8 + N, memory) >> (36 - N)) |
				    (comp->proc[i].op2mask & ~((1 << N) - 1)));
		    if(last->action == 0) { /* abort abort! we need to delete this action */
			/* go find action matching (all or one?) this addr */
			for(cur = trigs, fixup = trigs; cur->next != NULL; cur = cur->next) {
			    if(cur->addr == last->addr) {
				/* fix pointers and free */
				if(cur == trigs) { /* start of list */
				    ptrtmp = trigs->next;
				    free(trigs);
				    trigs = ptrtmp;
				} else {
				    ptrtmp = cur->next;
				    free(fixup->next);
				    fixup->next = ptrtmp;
				}
				cur = ptrtmp;
				if(cur->next == NULL)
				    break;
			    }
			    fixup = cur;
			}
			last->addr = 0;
		    } else {
			last->next = (struct trigger *)calloc(1, sizeof(struct trigger));
			if(last->next == NULL)
			    fatal("bad arm calloc\n");
			last = last->next;
		    }
		    comp->proc[i].ip += 8 + (2 * N);
		    break;
		case WAIT:
		    /* first check if any trigs are queued */
		    if(trigq->next != NULL) {
			struct trigger *ptrtmp = trigq->next;
			comp->proc[i].ip = trigq->action;
			free(trigq);
			trigq = ptrtmp;
			break;
		    }
		    comp->proc[i].run = 0;
		    /* should to put pinky on free list */
		    break;
		default:
		    fatal("bogus opcode\n");
		}
	    }
	}
    }
}

int main(int argc, char **argv) {
    struct pinky computer;
    size_t memsize;
    uint8 *memory;
    struct trigger *trigs, *trigq;
    int fd;

    if(argc!=4)
	fatal("wrong args\n");
    computer.numcpus = (size_t)atoi(argv[1]);
    memsize = (size_t)(atoi(argv[2]) * 1024 * 1024);
    fd = open(argv[3], O_RDWR);
    if(fd < 0)
	fatal("bad fd\n");
    /* Do I really want MAP_SHARED ? */
    memory = (uint8 *)mmap(0, memsize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(memory < 0)
	fatal("bad mmap\n");

    computer.proc = (struct pcpu *)calloc(computer.numcpus,
					  sizeof(struct pcpu));
    trigs = (struct trigger *)calloc(1, sizeof(struct trigger));
    trigq = (struct trigger *)calloc(1, sizeof(struct trigger));

    if((computer.proc == NULL) ||
       (trigs == NULL) ||
       (trigq == NULL))
	fatal("calloc failed\n");

    computer.proc[0].run = 1;
     
    p_run(&computer, memory, trigs, trigq);
}

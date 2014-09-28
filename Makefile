CFLAGS=-g -Wall
OBJS=pinky.o console.o number.o bitqueue.o memory.o farm.o asm.o p.o

all:	p

clean:
	rm -f $(OBJS) $(TARGET)

p:	$(OBJS)
	cc $(CFLAGS) $(OBJS) -o p

bitqueue.o: bitqueue.c bitqueue.h
console.o: console.c console.h number.h bitqueue.h pinky.h farm.h memory.h
pinky.o: pinky.c pinky.h number.h bitqueue.h farm.h memory.h
number.o: number.c number.h
memory.o: memory.c memory.h bitqueue.h
farm.o: farm.c farm.h number.h bitqueue.h pinky.h memory.h
asm.o: asm.c asm.h number.h bitqueue.h
p.o: p.c bitqueue.h number.h memory.h pinky.h farm.h console.h asm.h


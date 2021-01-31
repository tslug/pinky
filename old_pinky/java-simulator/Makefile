# Makefile for mikes prototype

MAKEFLAGS=-s
CJ = javac
RJ = java
CFLAGS=
RFLAGS=

DONTCOMPILE=

CLASSES := $(sort $(filter-out $(DONTCOMPILE),$(notdir $(patsubst %.java,%.class,$(wildcard *.java)))))

default: all

clear:
	clear

clean:
	rm -f *.class

all: clear $(CLASSES)

try: clear all
	java $(RFLAGS) PinkyAssembler pinky.asm

backup: clear clean
	rm -f Backup.tar.gz
	tar -cvf Backup.tar *
	gzip Backup.tar
	chmod 640 Backup.tar.gz



####################################################################
######################## BEGIN MISC BLOCK ##########################
####################################################################


######  objects:
%.class: %.java
	echo Compiling Module - $@
	$(CJ) $(patsubst %.class,%.java,$(notdir $@)) $(CFLAGS)


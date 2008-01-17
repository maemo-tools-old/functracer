CC = gcc
# debugging options
CFLAGS = -g -std=gnu99 -pedantic -Wall -Wconversion -Wshadow \
	-Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings \
	-Waggregate-return -Wstrict-prototypes

all: malloc_simple malloc_recursive fork clone

malloc_simple: malloc_simple.c
malloc_recursive: malloc_recursive.c
fork: fork.c
clone: clone.c

clean:
	rm -f malloc_simple malloc_recursive fork clone

distclean:
	rm -f *~

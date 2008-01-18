CC = gcc
# debugging options
CFLAGS = -g -std=gnu99 -pedantic -Wall -Wconversion -Wshadow \
	-Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings \
	-Waggregate-return -Wstrict-prototypes
CFLAGS += -mthumb

all: malloc_simple malloc_recursive fork clone gthreads

malloc_simple: malloc_simple.c
malloc_recursive: malloc_recursive.c
fork: fork.c
clone: clone.c

gthreads: gthreads.c
	$(CC) $(CFLAGS) $(shell pkg-config --cflags --libs /usr/lib/pkgconfig/gthread-2.0.pc) -o $@ $<

clean:
	rm -f malloc_simple malloc_recursive fork clone gthreads

distclean:
	rm -f *~

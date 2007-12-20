CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lelf

PROG = functracker

$(PROG): $(addsuffix .o, $(basename $(wildcard *.c)))

clean:
	rm -f *.o *~ $(PROG)

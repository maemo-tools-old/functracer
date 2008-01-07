CC = gcc
CFLAGS = -g -Wall
UNW_PREFIX = /usr/local/libunwind
CPPFLAGS = -iquote./include/ -I$(UNW_PREFIX)/include
LDFLAGS = -lelf -ldl -L$(UNW_PREFIX)/lib -lunwind-ptrace -lunwind-generic

UNAME = $(shell uname -m)
ifneq ($(filter i%86,$(UNAME)),)
ARCH = $(UNAME:i%86=i386)
# TODO ARM not supported yet
#else
#ifneq ($(filter arm%,$(UNAME)),)
#ARCH = $(UNAME:arm%=arm)
#endif
endif

PROG = functracker

all: $(PROG)
	$(MAKE) -C tests/ $@

$(PROG): $(addsuffix .o, $(basename $(wildcard *.c sysdeps/$(ARCH)/*.c)))
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) -o $@

tags: 
	ctags -R

clean:
	$(MAKE) -C tests/ $@
	rm -f *.o sysdeps/$(ARCH)/*.o $(PROG)

distclean: clean
	$(MAKE) -C tests/ $@
	rm -f core tags *~ include/*~

.PHONY: tags

CC = gcc
CFLAGS = -g -Wall
UNW_PREFIX = /usr/local/libunwind
LDFLAGS = -lbfd -L$(UNW_PREFIX)/lib -lunwind-ptrace -lunwind-generic

UNAME = $(shell uname -m)
ifneq ($(filter i%86,$(UNAME)),)

ARCH = $(UNAME:i%86=i386)
CPPFLAGS = -iquote./include/ -iquote./sysdeps/$(ARCH)/

else
ifneq ($(filter arm%,$(UNAME)),)

ARCH = $(UNAME:arm%=arm)
# gcc-3.4 does not support "-iquote"
CPPFLAGS = -I./include/ -I./sysdeps/$(ARCH)/
# workaround BFD linking issue
LDFLAGS += /usr/lib/libiberty.a

endif # ARM
endif # x86

CPPFLAGS += -I$(UNW_PREFIX)/include

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

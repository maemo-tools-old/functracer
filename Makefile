CC = gcc
CFLAGS = -g -Wall
CPPFLAGS = -iquote./include/
LDFLAGS = -lelf

UNAME = $(shell uname -m)
ifneq ($(filter i%86,$(UNAME)),)
ARCH = $(UNAME:i%86=i386)
else
ifneq ($(filter arm%,$(UNAME)),)
ARCH = $(UNAME:arm%=arm)
endif
endif

PROG = functracker

all: $(PROG)
	$(MAKE) -C tests/ $@

$(PROG): $(addsuffix .o, $(basename $(wildcard *.c sysdeps/$(ARCH)/*.c)))

tags: 
	ctags -R

clean:
	$(MAKE) -C tests/ $@
	rm -f *.o sysdeps/$(ARCH)/*.o $(PROG)

distclean: clean
	$(MAKE) -C tests/ $@
	rm -f tags *~

.PHONY: tags

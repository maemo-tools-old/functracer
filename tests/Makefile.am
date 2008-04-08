# Reset compiler/linker variables so that tests do not link with Functracer
# dependencies
# FIXME: temporary solution while dejagnu test framework is not integrated

CFLAGS =
LDFLAGS =
LIBS =

check_PROGRAMS = clone fork malloc_simple malloc_recursive callchain

callchain_LDADD = libcallchain.so

libcallchain.so: libcallchain.c
	$(CC) -shared -o $@ $<

clean-local:
	rm -f libcallchain.so

if GLIB_FOUND
check_PROGRAMS += gthreads
gthreads_CFLAGS = @GLIB_CFLAGS@
gthreads_LDFLAGS = @GLIB_LIBS@
endif

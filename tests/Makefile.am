check_PROGRAMS = clone fork malloc_simple malloc_recursive

if GLIB_FOUND
check_PROGRAMS += gthreads
gthreads_CFLAGS = @GLIB_CFLAGS@
gthreads_LDFLAGS = @GLIB_LIBS@
endif

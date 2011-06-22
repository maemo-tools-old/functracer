AM_CFLAGS = -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wno-unused-parameter 
SUBDIRS = modules
bin_PROGRAMS = functracer

functracer_SOURCES = functracer.c backtrace.c breakpoint.c callback.c	\
	debug.c dict.c maps.c options.c plugins.c process.c report.c 	\
	solib.c ssol.c target_mem.c trace.c util.c breakpoint-@ARCH@.c	\
	function-@ARCH@.c syscall-@ARCH@.c context.c filter.c

functracer_LDFLAGS = @FT_LIBS@ -rdynamic

bin_PROGRAMS = functracer

functracer_SOURCES = functracer.c breakpoint.c callback.c		\
	debug.c dict.c maps.c options.c process.c report.c solib.c	\
	target_mem.c trace.c util.c breakpoint-@ARCH@.c			\
	function-@ARCH@.c syscall-@ARCH@.c

if BACKTRACE_GDB

functracer_SOURCES += backtrace_gdb.c
functracer_CPPFLAGS = -I@GDB_SRCDIR@/gdb

else

functracer_SOURCES += backtrace.c

endif

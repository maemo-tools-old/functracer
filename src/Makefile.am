bin_PROGRAMS = functracker

functracker_SOURCES = functracker.c breakpoint.c callback.c		\
	debug.c dict.c maps.c options.c process.c report.c solib.c	\
	target_mem.c trace.c util.c breakpoint-@ARCH@.c			\
	function-@ARCH@.c syscall-@ARCH@.c

if BACKTRACE_GDB

functracker_SOURCES += backtrace_gdb.c
functracker_CPPFLAGS = -I@GDB_SRCDIR@/gdb

else

functracker_SOURCES += backtrace.c

endif

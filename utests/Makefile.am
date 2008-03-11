if CHECK_FOUND

AM_CFLAGS = -fprofile-arcs -ftest-coverage
MOSTLYCLEANFILES = *.bb *.bbg *.gcno
CLEANFILES = *.da *.gcda *.lcov

check_PROGRAMS = utest

utest_SOURCES = utest.c \
		utest_options.c \
		utest_options.h \
		utest_report.c \
		utest_report.h

utest_SOURCES += $(csourcedir)/breakpoint.c \
		 $(csourcedir)/callback.c \
		 $(csourcedir)/debug.c \
		 $(csourcedir)/dict.c \
		 $(csourcedir)/maps.c \
		 $(csourcedir)/process.c \
		 $(csourcedir)/solib.c \
		 $(csourcedir)/target_mem.c \
		 $(csourcedir)/trace.c \
		 $(csourcedir)/util.c \
		 $(csourcedir)/breakpoint-@ARCH@.c \
		 $(csourcedir)/function-@ARCH@.c \
		 $(csourcedir)/syscall-@ARCH@.c

utest_CPPFLAGS = $(CHECK_FLAGS) -I$(csourcedir)
utest_LDADD = $(CHECK_LIBS) $(LDADD)

if BACKTRACE_GDB
utest_SOURCES += $(csourcedir)/backtrace_gdb.c
utest_CPPFLAGS += -I@GDB_SRCDIR@/gdb
else
utest_SOURCES += $(csourcedir)/backtrace.c
endif

endif

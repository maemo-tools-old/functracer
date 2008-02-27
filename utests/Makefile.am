if CHECK_FOUND

AM_CFLAGS = -fprofile-arcs -ftest-coverage
MOSTLYCLEANFILES = *.bb *.bbg *.gcno
CLEANFILES = *.da *.gcda *.lcov

check_PROGRAMS = utest
utest_SOURCES = utest.c \
		utest_options.c \
		utest_options.h
utest_CPPFLAGS = $(CHECK_FLAGS) -I$(csourcedir)
utest_LDADD = $(CHECK_LIBS)
endif

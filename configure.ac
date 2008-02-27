AC_INIT([functracker], [0.3])
AC_CONFIG_HEADERS(include/config.h)
AC_CONFIG_SRCDIR(src/functracker.c)
AM_INIT_AUTOMAKE([-Wall -Werror foreign no-define])
AC_PROG_CC

AC_FUNC_CLOSEDIR_VOID
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([memset strdup strerror])

AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([limits.h stddef.h])

AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIGNAL
AC_TYPE_SIZE_T

CPPFLAGS="${CPPFLAGS} -D_GNU_SOURCE"

# Ask user for path to libunwind
AC_ARG_WITH(libunwind,
  [AS_HELP_STRING([--with-libunwind],
    [prefix of libunwind installation (required for backtracing on x86)])],
  [UNW_FLAGS="-I$withval/include -L$withval/lib"],
  [UNW_FLAGS=""]
)
CFLAGS="${CFLAGS} ${UNW_FLAGS}"

# Ask user for path to GDB sources
AC_ARG_WITH(gdb,
  [AS_HELP_STRING([--with-gdb],
    [location of compiled GDB sources (required for backtracing on ARM)])],
  [GDB_SRCDIR="`readlink -f $withval`"],
  [GDB_SRCDIR=""]
)

# Ask user for enable utests with check (http://check.sourceforge.net)
AC_ARG_ENABLE(check,
  [AS_HELP_STRING([--enable-check],
    [enable utests with Check utest framework])],
    [enable_check=yes],
    [enable_check=no]
)

if test -z "${GDB_SRCDIR}"; then

# Check for libunwind availability
AC_CHECK_LIB([unwind-ptrace], [_UPT_access_mem],
  [LIBS="${LIBS} -lunwind-ptrace -lunwind-generic"],
  [AC_MSG_ERROR([libunwind is required for backtracing])],
  -lunwind-generic
)

# Check for BFD availability
# Linking with libiberty.a is necessary due to Scratchbox bug
AC_CHECK_LIB([bfd], [bfd_fopen],
  [LIBS="${LIBS} -lbfd /usr/lib/libiberty.a"],
  [AC_MSG_ERROR([BFD library is required])],
  /usr/lib/libiberty.a
)

else

# Check for libreadline availability
AC_CHECK_LIB([readline], [tilde_expand],
  [LIBS="${LIBS} -lreadline"],
  [AC_MSG_ERROR([libreadline is required for backtracing on ARM])],
  dnl empty
)

# Check for libexpat availability
AC_CHECK_LIB([expat], [XML_ParserCreateNS],
  [LIBS="${LIBS} -lexpat"],
  [AC_MSG_ERROR([libexpat is required for backtracing on ARM])],
  dnl empty
)

CFLAGS="${CFLAGS} -L${GDB_SRCDIR}/gdb -L${GDB_SRCDIR}/bfd -L${GDB_SRCDIR}/opcodes"
CPPFLAGS="${CPPFLAGS} -I${GDB_SRCDIR} -I${GDB_SRCDIR}/include -I${GDB_SRCDIR}/gdb/config"

# Check for libgdb.a availability
AC_CHECK_LIB([gdb], [get_current_frame],
  [LIBS="${LIBS} -lgdb -lbfd -lopcodes -lm -ldl /usr/lib/libiberty.a"],
  [AC_MSG_ERROR([libgdb.a is required for backtracing on ARM])],
  -lbfd -lopcodes -lm -ldl /usr/lib/libiberty.a
)

fi

AM_CONDITIONAL(BACKTRACE_GDB, [test "x${GDB_SRCDIR}" != "x"])
AC_SUBST([GDB_SRCDIR])

# Check for glib-2.0 availability (used for gthreads test)
PKG_PROG_PKG_CONFIG
AS_IF([test "x${PKG_CONFIG}" != "x"],
  [PKG_CHECK_MODULES(GLIB, gthread-2.0,
    [AC_SUBST(GLIB_CFLAGS) AC_SUBST(GLIB_LIBS)],
    [AC_MSG_WARN(glib-2.0 not found, gthreads test will not be compiled)])],
  [AC_MSG_WARN(pkg-config not found, gthreads test will not be compiled)]
)

if test "x$enable_check" = "xyes"; then

AS_IF([test "x${PKG_CONFIG}" != "x"],
  [PKG_CHECK_MODULES(CHECK, check,
    [AC_SUBST(CHECK_CFLAGS) AC_SUBST(CHECK_LIBS) AC_SUBST(csourcedir, ../$srcdir/src)],
    [AC_MSG_WARN(check not found, utests will not be compiled)])],
  [AC_MSG_WARN(pkg-config not found, utests will not be compiled)],
)

fi

AM_CONDITIONAL(CHECK_FOUND, [test "x${CHECK_CFLAGS}" != "x"])
AM_CONDITIONAL(GLIB_FOUND, [test "x${GLIB_CFLAGS}" != "x"])

# Detect architecture
AC_CANONICAL_HOST
AS_CASE($host_cpu,
  [arm*], [ARCH=arm; max_bt_depth=12; max_nalloc=32768],
  [i?86], [ARCH=i386; max_bt_depth=64; max_nalloc=65536],
  [AC_MSG_ERROR([host CPU $host_cpu is not supported])]
)
AC_SUBST([ARCH])

# Define architecture-specific constants used in include/report.h
AC_DEFINE_UNQUOTED([MAX_BT_DEPTH], [$max_bt_depth], Define maximum backtrace depth.)
AC_DEFINE_UNQUOTED([MAX_NALLOC], [$max_nalloc], Define maximum number of allocations.)

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  tests/Makefile
  utests/Makefile
])

AC_OUTPUT

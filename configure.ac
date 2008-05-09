AC_INIT([functracer], [0.4])
AC_CONFIG_HEADERS(include/config.h)
AC_CONFIG_SRCDIR(src/functracer.c)
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
    [prefix of libunwind installation (required for backtracing)])],
  [UNW_FLAGS="-I$withval/include -L$withval/lib"],
  [UNW_FLAGS=""]
)
CFLAGS="${CFLAGS} ${UNW_FLAGS}"


# Ask user for enable utests with check (http://check.sourceforge.net)
AC_ARG_ENABLE(check,
  [AS_HELP_STRING([--enable-check],
    [enable utests with Check utest framework])],
    [enable_check=yes],
    [enable_check=no]
)

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
  [arm*], [ARCH=arm; max_bt_depth=12],
  [i?86], [ARCH=i386; max_bt_depth=64],
  [AC_MSG_ERROR([host CPU $host_cpu is not supported])]
)
AC_SUBST([ARCH])

# Define architecture-specific constants used in include/report.h
AC_DEFINE_UNQUOTED([MAX_BT_DEPTH], [$max_bt_depth], Define maximum backtrace depth.)

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  tests/Makefile
  utests/Makefile
])

AC_OUTPUT

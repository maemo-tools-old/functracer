AC_INIT([functracker], [0.3])
AC_CONFIG_HEADERS(include/config.h)
AM_INIT_AUTOMAKE([-Wall -Werror foreign no-define])
AC_PROG_CC

# Ask user for path to libunwind
AC_ARG_WITH(libunwind,
  [AS_HELP_STRING([--with-libunwind],
    [prefix of libunwind installation, e.g. /usr/local or /usr])],
  [UNW_FLAGS="-I$withval/include -L$withval/lib"],
  [UNW_FLAGS=""]
)
CFLAGS="$CFLAGS $UNW_FLAGS"

# Check for libunwind availability
AC_CHECK_LIB([unwind-ptrace], [_UPT_access_mem],
  [LIBS="$LIBS -lunwind-ptrace -lunwind-generic"],
  [AC_MSG_ERROR([libunwind is required for backtracing])],
  -lunwind-generic
)

# Check for BFD availability
# Linking with libiberty.a is necessary due to Scratchbox bug
AC_CHECK_LIB([bfd], [bfd_fopen],
  [LIBS="$LIBS -lbfd /usr/lib/libiberty.a"],
  [AC_MSG_ERROR([BFD library is required])],
  /usr/lib/libiberty.a
)

# Check for glib-2.0 availability (used for gthreads test)
PKG_PROG_PKG_CONFIG
AS_IF([test "x$PKG_CONFIG" != "x"],
  [PKG_CHECK_MODULES(GLIB, gthread-2.0,
    [AC_SUBST(GLIB_CFLAGS) AC_SUBST(GLIB_LIBS)],
    [AC_MSG_WARN(glib-2.0 not found, gthreads test will not be compiled)])],
  [AC_MSG_WARN(pkg-config not found, gthreads test will not be compiled)]
)
AM_CONDITIONAL(GLIB_FOUND, [test "x$GLIB_CFLAGS" != "x"])

# Detect architecture
AC_CANONICAL_HOST
case $host_cpu in
  arm*) ARCH=arm ;;
  i?86) ARCH=i386 ;;
  *) AC_MSG_ERROR([host CPU $host_cpu is not supported]) ;;
esac
AC_SUBST([ARCH])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  tests/Makefile
])

AC_OUTPUT

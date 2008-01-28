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
])

AC_OUTPUT

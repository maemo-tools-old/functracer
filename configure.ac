AC_INIT([functracer], [1.3.4])
AC_CONFIG_HEADERS(include/config.h)
AC_CONFIG_SRCDIR(src/functracer.c)
AM_INIT_AUTOMAKE([-Wall -Werror foreign no-define])
AC_PROG_CC
AC_PROG_LIBTOOL

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
if test x$enable_debug = xyes; then
  CPPFLAGS="${CPPFLAGS} -DDEBUG"
fi

if test x$prefix = xNONE; then
  prefix=$ac_default_prefix
fi

# Ask user for path to libunwind
AC_ARG_WITH(libunwind,
  [AS_HELP_STRING([--with-libunwind],
    [prefix of libunwind installation (required for backtracing)])],
  [UNW_FLAGS="-I$withval/include -L$withval/lib"],
  [UNW_FLAGS=""]
)
CFLAGS="${CFLAGS} ${UNW_FLAGS}"

# Ask user for path to plugins
AC_ARG_WITH(plugins,
  [AS_HELP_STRING([--with-plugins],
    [prefix of plugins installation (default=PREFIX/lib/functracer)])],
  [plg_path="$withval"],
  [plg_path="${prefix}/lib/functracer"]
)
AC_DEFINE_UNQUOTED([PLG_PATH], ["$plg_path"], Prefix of plugins installation.)

# Turn on debug support 
AC_ARG_ENABLE(debug,
[  --enable-debug		turn on debug support (slow down execution)],
[enable_debug=yes], [])

# Check for libunwind availability
AC_CHECK_LIB([unwind-ptrace], [_UPT_access_mem],
  [FT_LIBS="${FT_LIBS} -lunwind-ptrace -lunwind-generic"],
  [AC_MSG_ERROR([libunwind is required for backtracing])],
  -lunwind-generic
)

# Check for dlopen
AC_CHECK_LIB([dl], [dlopen],
  [LIBS_DL="-ldl"],
  [AC_MSG_ERROR([dlopen library is required for plugins])],
)
FT_LIBS="${FT_LIBS} $LIBS_DL"

# Check for zlib availability
AC_CHECK_LIB([z], [inflate],
  [LIBS_Z="-lz"],
  [AC_MSG_ERROR([zlib library is required])],
)

# Check for liberty_pic
AC_CHECK_LIB([iberty_pic], [cplus_demangle],
 [LIBS_IBERTY="-liberty_pic"],
 [LIBS_IBERTY="-liberty"]
)

# Check for BFD availability
# Linking with libiberty.a is necessary due to Scratchbox bug
AC_CHECK_LIB([bfd], [bfd_fopen],
  [FT_LIBS="${FT_LIBS} -lbfd $LIBS_IBERTY $LIBS_DL $LIBS_Z"],
  [AC_MSG_ERROR([BFD library is required: $LIBS_DL $LIBS_Z $LIBS_IBERTY])],
  [$LIBS_DL $LIBS_Z $LIBS_IBERTY]
)

# Hack: "bfd_hide_sym_by_version" function has been introduced
#       in binutils-2.22, (ab)use this for the version check
AC_CHECK_LIB([bfd], [bfd_hide_sym_by_version],
  [CPPFLAGS="${CPPFLAGS} -DHAVE_BINUTILS_2_22_OR_NEWER"]
)

# Check for libsp-rtrace1 libary
AC_CHECK_LIB([sp-rtrace1], [sp_rtrace_print_comment],
  [FT_LIBS="${FT_LIBS} -lsp-rtrace1"],
  [AC_MSG_ERROR([libsp-rtrace1 library is required for output formatting])],
)

AC_SUBST(FT_LIBS)

# Check for glib-2.0 availability (used for gthreads test)
PKG_PROG_PKG_CONFIG
AS_IF([test "x${PKG_CONFIG}" != "x"],
  [PKG_CHECK_MODULES(GLIB, gthread-2.0,
    [AC_SUBST(GLIB_CFLAGS) AC_SUBST(GLIB_LIBS)],
    [AC_MSG_WARN(glib-2.0 not found, gthreads test will not be compiled)])],
  [AC_MSG_WARN(pkg-config not found, gthreads test will not be compiled)]
)

AM_CONDITIONAL(GLIB_FOUND, [test "x${GLIB_CFLAGS}" != "x"])

# Detect architecture
AC_CANONICAL_HOST
AS_CASE($host_cpu,
  [arm*], [ARCH=arm; AC_DEFINE([ARCH_ARM], [], [ARM architecture.])],
  [i?86], [ARCH=i386]; AC_DEFINE([ARCH_X86], [], [x86 architecutre.]),
  [AC_MSG_ERROR([host CPU $host_cpu is not supported])]
)
AC_SUBST([ARCH])

# Define build architecture
AC_DEFINE_UNQUOTED([BUILD_ARCH], "$(uname -m)", Build architecture.)

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/modules/Makefile
  testsuite/Makefile
  testsuite/functracer.file/Makefile
  testsuite/functracer.main/Makefile
  testsuite/functracer.memory/Makefile
  testsuite/functracer.memtransfer/Makefile
  testsuite/functracer.shmsysv/Makefile
  testsuite/functracer.audit/Makefile
  testsuite/functracer.gobject/Makefile
  testsuite/functracer.thread/Makefile
  testsuite/functracer.shmposix/Makefile
])

AC_OUTPUT

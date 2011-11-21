Name: functracer
Version: 1.3.3
Release: 1%{?dist}
Summary: A Function Backtrace Tool
Group: Development/Tools
License: GPLv2+	
URL: http://www.gitorious.org/+maemo-tools-developers/maemo-tools/functracer
Source: %{name}_%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-build
BuildRequires: autoconf, automake, pkg-config, help2man, libtool, binutils-devel, libunwind-devel
BuildRequires: libsp-rtrace-devel, zlib-devel

%description
 Functracer is a debugging tool. It collects backtraces, arguments and return
 value of functions specified in a plugin. It works with optimized (-O2) code,
 where debugging symbols are available either in the application binaries
 themselves, or in separate debug files.
 
%prep
%setup -q -n functracer

%build
autoreconf -fvi

%configure 

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
rm %{buildroot}/usr/lib/%{name}/*a


%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_bindir}/functracer
%{_libdir}/%{name}/*so
%{_mandir}/man1/functracer.1.gz
%doc README COPYING src/modules/TODO.plugins src/modules/README.plugins


%changelog
* Wed Oct 19 2011 Eero Tamminen <eero.tamminen@nokia.com> 1.3.3
  * Fix program start breakpoint handling.

* Wed Oct 12 2011 Eero Tamminen <eero.tamminen@nokia.com> 1.3.2
  * Add support for pthreads created as detached.

* Fri Sep 23 2011 Eero Tamminen <eero.tamminen@nokia.com> 1.3.1
  * Manual page fix.

* Tue Aug 23 2011 Eero Tamminen <eero.tamminen@nokia.com> 1.3
  * Replace 'verbose' option with 'quiet' to provide by default more state
    information output.
  * Increase maximum backtrace depth to 256.

* Wed Aug 03 2011 Eero Tamminen <eero.tamminen@nokia.com> 1.2.1
  * Fix audit module symbol parsing:
    - Use ';' as symbol separator,
    - Allow shell style pattern matching for symbol names.

* Thu Jun 30 2011 Eero Tamminen <eero.tamminen@nokia.com> 1.2
  * Fix target process crashes on detaching.
  * Use debug symbols for audit (-a) option.
  * Add -L option to specify libraries to scan.
  * Update command line  options to be consistent with sp-rtrace
      -b  ->  -A
      -l  ->  -o
      -t  ->  -b
  * Change disable timestamps option to -T.

* Wed Jun 01 2011 Eero Tamminen <eero.tamminen@nokia.com> 1.1
  * Fix crashes sometimes seen during process detach.
  * New option --monitor: Report backtraces only for allocations having the
    specified resource size.
  * Depends on sp-rtrace 1.6.

* Fri Mar 04 2011 Eero Tamminen <eero.tamminen@nokia.com> 1.0.1
  * Fix functracer spinning forever or doing double free when it
    detaches from (threading) gst-launch

* Wed Feb 16 2011 Eero Tamminen <eero.tamminen@nokia.com> 1.0
  * increase backtrace max depth from 12 to 64.
  * compiler option, x86 reliability and minor memory leak fixes.
  * use proper trace headings.
  * emulate ARM instructions required by function entry points
    traced by different modules.
  * add module for tracking POSIX shared memory mappings.
  * misc improvements to already existing modules.
  * -a option enables audit module automatically and supports partial matches.
  * test-suite updated to latest state + tests added for all modules.
  * module documentation improvements and more module generator examples.
  * enabled timestamps by default. Disable with -I (--no-time) switch.

* Fri Dec 10 2010 Eero Tamminen <eero.tamminen@nokia.com> 0.23
  * Added pipe2 function tracking to file module. 
  * Added audit module supporting custom tracking symbol list.
  * Updated to match sp-rtrace API changes.

* Fri Nov 12 2010 Eero Tamminen <eero.tamminen@nokia.com> 0.22
  * Fix crash when target process calls sp-rtrace context function while
    functracer is not in tracing mode. 

* Mon Oct 25 2010 Eero Tamminen <eero.tamminen@nokia.com> 0.21
  * Added shared memory module "shmsysv" for tracking shmget(), shmctl(),
    shmdt() and shmat() calls.
  * sp-rtrace function call context support.

* Fri Sep 24 2010 Eero Tamminen <eero.tamminen@nokia.com> 0.20
  * Use sp-rtrace compatible output format. 
  * Update README.

* Fri Feb 12 2010 Eero Tamminen <eero.tamminen@nokia.com> 0.19
  * Fix SIGINT/SIGTERM signals handling. 
  * Huge update:
    + fix handling of processes and threads
    + functracer module to track GObject allocations and releases
    + functracer module to track memory transfer operations
    + memory: suppress allocation failures and track valloc
    + file: track more calls and enhance output
    + functracer module to keep track on memory allocation/release
      caused by creating/joining/detaching threads
    + update documentation

* Tue Oct 13 2009 Eero Tamminen <eero.tamminen@nokia.com> 0.18
  * Change license of functracer to GPL version 2 or later.

* Tue Jun 09 2009 Eero Tamminen <eero.tamminen@nokia.com> 0.17
  * Fix segfault on attaching to non-main thread
  * Update version in configure.ac

* Thu May 07 2009 Eero Tamminen <eero.tamminen@nokia.com> 0.16
  * Rewrite processes and threads hangling to avoid assumptions about ptrace's
    messages order. It should work correctly on Fremantle now.

functracer (0.15.1maemo1), 17 Feb 2009:
  * Fix a crash occurring when process does not yet have parent
    information available. 
  * Applied a patch to fix realpath() portability problems by using
    readlink() instead. 

* Thu Jan 29 2009 Eero Tamminen <eero.tamminen@nokia.com> 0.15.1
  * Small fixes in documentation: README, README.plugins, TODO.plugins and
    manpage.

* Tue Jan 20 2009 Eero Tamminen <eero.tamminen@nokia.com> 0.15
  * Plugins cleanup.
  * Add a new event to fork in parent process.
  * Do not read symbols from the target program and dynamic linker.
  * Add new functions to file plugin.
  * Change info messages to show "process/thread".
  * Fix date shown in trace header.
  * Add new specific documentation to plugins.
  * Add a README to testsuite.

* Fri Dec 19 2008 Eero Tamminen <eero.tamminen@nokia.com> 0.14
  * Add verbose option.
  * Avoid ptrace() errors due to missing libraries.
  * Change minimal backtrace depth to zero.
  * Organize testsuite directories.
  * Fix leaks in clone and fork tests.
  * Add new flags and fix warnings in testsuite.
  * Add support to C++ programs on testsuite.
  * Add new test to C++ programs in testsuite.
  * Add "Functracer Internals" sources.
  * Testsuite improvements.
  * Add new tests for calloc, realloc and memalign functions.
  * Add new tests for open/close operations.
  * Modify instructions for SSOL support.
  * Debian packaging fixes.
  * Move architecture specific defines out of configure.ac.
  * x86: allow inserting breakpoints in instructions with size 8.
  * Memory plugin fixes (memalign, posix_memalign).
  * solib.c: ignore symbols with no defined address.
  * Fix copyright header in testsuite files.

* Thu Dec 04 2008 Eero Tamminen <eero.tamminen@nokia.com> 0.13
  * Add a new exec event on callback.
  * Add new data to trace file header.
  * Add support to timestamps.
  * Remove unnecessary check to lock functions.
  * Add a sequence to load plugin module.
  * Change plugin infrastructure.
  * Rename tests directory to testsuite.
  * Implement support to dejagnu test framework.
  * Remove utests directory.

* Wed Nov 12 2008 Eero Tamminen <eero.tamminen@nokia.com> 0.12
  * Add initial support to plugins.
  * Add new option "-e" to plugin name.
  * Add in configure a new entry "--with-plugins" to prefix of plugins
    installation.
  * Add check in xptrace() function.
  * Change the backtrace header format for a more generic one.
  * Avoid overwriting trace file.
  * Simplify dump control functions.
  * Handle interrupt before a singlestep action.

* Mon Oct 20 2008 Eero Tamminen <eero.tamminen@nokia.com> 0.11
  * Fix SSOL area creation after a exec*() on ARM.
  * Remove /proc/PID/maps copying functions.
  * Dump maps information to trace file.
  * Replace get_list_of_processes() with for_each_process().
  * Fix regression in dump control; indentation fixes.

* Wed Oct 01 2008 Eero Tamminen <eero.tamminen@nokia.com> 0.10
  * Add prefix "__" on pthread_mutex_lock/pthread_mutex_unlock functions.
  * Fix allocation report for multithread programs.
  * Add missing *fPIC flag to libcallchain.so build.
  * Update .gitignore.
  * Remove unused declarations.
  * Fix memory leak in remove_process().
  * Simplify xptrace().
  * Change backtrace output format.
  * Add second line to header.
  * Small refactorings and a race condition fix.
  * Fix functracer interrupt (CTRL+C) handling.
  * Improve some tests.
  * Replace traditional breakpoint approach with SSOL.
  * Add support for reporting of process events.

* Fri Sep 12 2008 Eero Tamminen <eero.tamminen@nokia.com> 0.9.2
  * Fix some lintian warnings, copyrights and README.
  * Change option letter for help messagei (us -h).
  * Create man page automatically.
  * Change addresses to have same width.
  * Replace TODO/XXX comments for FIXME.
  * Avoid reporting internal/recursive calls.
  * Generate only one .trace file per process.
  * Trace pthread_mutex_lock/pthread_mutex_unlock functions.

* Fri Aug 08 2008 Eero Tamminen <eero.tamminen@nokia.com> 0.9.1
  * Fix segfault when non-write permission is received.
  * Fix race condition on get_process_state().
  * Move ft_wait*() functions to right place.

* Fri Aug 01 2008 Eero Tamminen <eero.tamminen@nokia.com> 0.9
  * Attach all processes/threads when using -p option.
  * Avoid enable or disable breakpoints more than once.
  * Share breakpoints between parent and children.
  * Fix verification for repeated return addresses.
  * Add support for multithreaded programs.

* Fri Jul 04 2008 Eero Tamminen <eero.tamminen@nokia.com> 0.8
  * Add a header to trace file.
  * Reorganize report functions.
  * Add option to enable address resolution on functracer.
  * Add option to enable backtraces for free().
  * Fix compilation warnings.
  * Remove "name" variable from debug message.
  * Added error check on unw_get_proc_name().
  * Update documentation (README, manpage).
  * Decrement IP value to get correct line on post-processing.

* Fri Jun 20 2008 Eero Tamminen <eero.tamminen@nokia.com> 0.7
  * Name resolution (unw_get_proc_name) removed. 

* Fri Jun 13 2008 Eero Tamminen <eero.tamminen@nokia.com> 0.6
  * Enable realloc() tracing.
  * Change ptrace for using PTRACE_CONT.
  * Remove remaining handling to SIGUSR2.
  * Fix segfault when permission denied is received.
  * Fix segfault when more than one process is tracked.
  * Enable libunwind cache (unw_set_caching_policy).
  * Suppress "free(NULL)" calls from trace output.

* Mon Jun 02 2008 Eero Tamminen <eero.tamminen@nokia.com> 0.5
  * Add debug package creation.
  * Add the __libc_ prefix for tracing functions.
  * Move rp_copy_maps() to rp_finish().
  * Simplify reporting functions.
  * Support for enabling debug during compilation.
  * Fix dump control regression (just toggle the tracing on/off).
  * Update documentation.

* Tue May 06 2008 Eero Tamminen <eero.tamminen@nokia.com> 0.4
  * Initial release.

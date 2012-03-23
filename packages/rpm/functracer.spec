Name: functracer
Version: 1.3.4
Release: 1%{?dist}
Summary: A Function Backtrace Tool
Group: Development/Tools
License: GPLv2+	
URL: http://www.gitorious.org/+maemo-tools-developers/maemo-tools/functracer
Source: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-build
BuildRequires: autoconf, automake, pkg-config, help2man, libtool, binutils-devel, libunwind-devel
BuildRequires: libsp-rtrace-devel

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
%{_libdir}/%{name}/
%{_mandir}/man1/functracer.1.gz
%doc README COPYING src/modules/TODO.plugins src/modules/README.plugins


%changelog
* Fri Dec 23 2011 Eero Tamminen <eero.tamminen@nokia.com> 1.3.4
  * Fix crash when output directory is not accessible.
  * Spec tweaks

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

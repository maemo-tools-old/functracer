Name: functracer
Version: 1.3.2
Release: 1%{?dist}
Summary: A Function Backtrace Tool
Group: Development/Tools
License: GPLv2+	
URL: http://www.gitorious.org/+maemo-tools-developers/maemo-tools/functracer
Source: %{name}_%{version}.tar.gz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
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
make 

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
rm %{buildroot}/usr/lib/%{name}/*a


%clean
rm -rf %{buildroot}

%files
%defattr(755,root,root,-)
%{_bindir}/functracer
%defattr(644,root,root,-)
%{_libdir}/%{name}/*so
%{_mandir}/man1/functracer.1.gz
%doc README COPYING src/modules/TODO.plugins src/modules/README.plugins


# spec file for package mxflib
#
# Copyright (c) 2003 Stuart Cunningham
# stuart_hc@users.sourceforge.net
#

Name:			mxflib-beta
Summary:		C++ MXF file I/O library
Version:		0.3.3
Release:		1
Group:			Development/Libraries/C and C++
BuildPreReq:	e2fsprogs-devel
License:		zlib/libpng License
URL:			http://sourceforge.net/projects/mxflib
Source0:		mxflib-beta-0.5.0.tar.gz
BuildRoot:		/var/tmp/%{name}-buildroot

%description
MXFLib is a C++ library providing support for the MXF file format
which is standardised as SMPTE 377M.  Included with the library
are a number of simple example applications:
  mxfdump
  mxfwrap
  mxfsplit

%prep
rm -rf $RPM_BUILD_ROOT

%setup

%build

# _prefix is usually /usr
./configure --prefix=%{_prefix}
make

%install

make DESTDIR=$RPM_BUILD_ROOT install

%clean
rm -fr $RPM_BUILD_ROOT

%files
%doc AUTHORS COPYING ChangeLog NEWS README
%{_prefix}/bin/*
%{_prefix}/share/mxflib
%{_prefix}/share/doc/mxflib
%{_prefix}/lib/*
%{_prefix}/include

%changelog -n mxflib
* Fri Apr 30 2004 - stuart_hc@users.sourceforge.net
- mxflib no longer requires klvlib
- include HTML documentation in binary rpms

* Fri Dec  5 2003 - stuart_hc@users.sourceforge.net
- First release of rpm package for mxflib

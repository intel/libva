#%define moduledir %(pkg-config xorg-server --variable=moduledir)
%define libversion 1.0.11

Name:           libva
Version:        %{libversion}
Release:        0.0
License:        MIT
Source:         %{name}-%{version}.tar.bz2
NoSource:	0
Group:          Development/Libraries
Summary:        Video Acceleration (VA) API for Linux
URL:            http://freedesktop.org/wiki/Software/vaapi
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

Requires: xorg-x11-server-Xorg

Requires: /sbin/ldconfig
BuildRequires:  pkgconfig(xv)
BuildRequires:  pkgconfig(xrandr)
BuildRequires:  pkgconfig(xorg-server)
BuildRequires:  pkgconfig(libdrm)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(xext)
BuildRequires:  pkgconfig(xdamage)
BuildRequires:  pkgconfig(xfixes)
BuildRequires:  pkgconfig(dri2proto)
BuildRequires:  pkgconfig(damageproto)
BuildRequires:  pkgconfig(kbproto)
BuildRequires:  pkgconfig(xextproto)
BuildRequires:  pkgconfig(fixesproto)
BuildRequires:  pkgconfig(xproto)
BuildRequires:  pkgconfig(gl)
BuildRequires:  libtool





%description
The libva library implements the Video Acceleration (VA) API for Linux.
The library loads a hardware dependendent driver.

%package devel
Summary: Video Acceleration (VA) API for Linux -- development files
Group:          Development/Libraries
Requires: %{name} = %{version}-%{release}
Requires: pkgconfig

%description devel
The libva library implements the Video Acceleration (VA) API for Linux.
The library loads a hardware dependendent driver.

This package provides the development environment for libva.

%prep
%setup -q

%build
unset LD_AS_NEEDED
%autogen
make

%install
%make_install

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/libva.so.1
%{_libdir}/libva.so.%{libversion}
%{_libdir}/libva-tpi.so.1
%{_libdir}/libva-tpi.so.%{libversion}
%{_libdir}/libva-x11.so.1
%{_libdir}/libva-x11.so.%{libversion}
%{_libdir}/libva-glx.so.1
%{_libdir}/libva-glx.so.%{libversion}
%{_libdir}/libva-egl.so.1
%{_libdir}/libva-egl.so.%{libversion}
%{_bindir}/vainfo
%{_bindir}/test_*
%{_bindir}/h264encode
%{_bindir}/mpeg2vldemo
%{_bindir}/putsurface

%{_libdir}/dri/dummy_drv_video.so

%files devel
%defattr(-,root,root,-)
%dir %{_includedir}/va
%{_includedir}/va/*
%{_libdir}/libva.so
%{_libdir}/libva-tpi.so
%{_libdir}/libva-x11.so
%{_libdir}/libva-glx.so
%{_libdir}/libva-egl.so
%{_libdir}/pkgconfig/libva.pc
%{_libdir}/pkgconfig/libva-tpi.pc
%{_libdir}/pkgconfig/libva-x11.pc
%{_libdir}/pkgconfig/libva-glx.pc
%{_libdir}/pkgconfig/libva-egl.pc

%changelog
* Tue Jan 25 2011 Austin Yuan <shengquan.yuan@intel.com> 1.0.1
- Updated libva source to 1.0.8
* Wed Dec 23 2009 Prajwal Mohan <prajwal.karur.mohan@intel.com> 1.0.1
- Updated libva source to IMG Alpha7
* Fri Dec 18 2009 Prajwal Mohan <prajwal.karur.mohan@intel.com> 0.31.0
- Updated libva source to IMG Alpha6
* Tue Nov 24 2009 Prajwal Mohan <prajwal.karur.mohan@intel.com> 0.31.0
- Updated libva source
* Mon Nov  9 2009 Prajwal Mohan <prajwal.karur.mohan@intel.com> 0.31.0
- Update to version 0.31.0
* Tue Jul  7 2009 Prajwal Mohan <prajwal.karur.mohan@intel.com> 0.30.4
- Update to version 0.30.4
* Wed Jun 24 2009 Prajwal Mohan <prajwal.karur.mohan@intel.com> 0.30.20090618
- Update to version 0.30.20090618_Alpha2.3
* Thu Jun 11 2009 Priya Vijayan <priya.vijayan@intel.com> 0.30.20090608
- Update to version 0.30.20090608
* Tue Jun  2 2009 Anas Nashif <anas.nashif@intel.com> - 0.30~20090514
- unset LD_AS_NEEDED
* Fri May 15 2009 Anas Nashif <anas.nashif@intel.com> 0.30~20090514
- Update to latest snapshot 20090514
* Wed Apr 29 2009 Anas Nashif <anas.nashif@intel.com> 0.30~20090428
- Update to 20090428 snpashot
* Sun Apr 26 2009 Anas Nashif <anas.nashif@intel.com> 0.30~20090423
- Update libva.pc with new includedir
* Sun Apr 26 2009 Anas Nashif <anas.nashif@intel.com> 0.30~20090423
- Update to latest snapshot 20090423
* Sun Apr 26 2009 Anas Nashif <anas.nashif@intel.com> 0.30~20090423
- Update to latest snapshot: 20090423
* Mon Mar 23 2009 Anas Nashif <anas.nashif@intel.com> 0.30~20090323
- Update to 0.30~20090323
* Fri Jan 16 2009 Priya Vijayan <priya.vijayan@intel.com> 0.31
- Fixing vainfo.c
* Fri Jan 16 2009 Priya Vijayan <priya.vijayan@intel.com> 0.31
- fixing va_backend.h
* Fri Jan 16 2009 Priya Vijayan <priya.vijayan@intel.com> 0.31
- Updating source
* Tue Jan  6 2009 Priya Vijayan <priya.vijayan@intel.com> 0.30
- Update to 0.28-working combination with X Server
* Fri Dec 19 2008 Priya Vijayan <priya.vijayan@intel.com> 0.30.0
- Added definition of FOURCC IYUV
  * Dec 16 2008 Priya Vijayan <priya.vijayan@intel.com>
- Initial Import to MRST
* Mon Oct 13 2008 shengquan.yuan@intel.com
- packaged mrst-video-decode-src version 0.0.1 using the buildservice spec file wizard

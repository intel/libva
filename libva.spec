#%define moduledir %(pkg-config xorg-server --variable=moduledir)
%define driverdir %{_libdir}/dri

%define reldate 04282009

Name:           libva
Version:        1.0.4
Release:        0.0
License:        MIT
Source:         %{name}-%{version}.tar.bz2
Group:          Development/Libraries
Summary:        Video Acceleration (VA) API for Linux
URL:            http://freedesktop.org/wiki/Software/vaapi
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

Requires: xorg-x11-server-Xorg

BuildRequires: libtool xorg-x11-server-devel pkgconfig(xv) pkgconfig(xrandr)
BuildRequires: libdrm-devel libX11-devel libXext-devel libXdamage-devel libXfixes-devel xorg-x11-proto-dri2proto
BuildRequires: xorg-x11-proto-damageproto xorg-x11-proto-kbproto xorg-x11-proto-xproto xorg-x11-proto-xextproto xorg-x11-proto-fixesproto

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
rm -rf $RPM_BUILD_ROOT
%make_install
mkdir -p $RPM_BUILD_ROOT%{driverdir}
install -m 755 ./dummy_drv_video/.libs/dummy_drv_video.so $RPM_BUILD_ROOT%{driverdir}/dummy_drv_video.so

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/libva.so.1
%{_libdir}/libva.so.1.0.4
%{_libdir}/libva-tpi.so.1
%{_libdir}/libva-tpi.so.1.0.4
%{_libdir}/libva-x11.so.1
%{_libdir}/libva-x11.so.1.0.4
%{_bindir}/vainfo
%{_bindir}/test_*
%{_bindir}/h264encode
%{_bindir}/mpeg2vldemo
%{_bindir}/putsurface
%{driverdir}/dummy_drv_video.so

%files devel
%defattr(-,root,root,-)
%dir %{_includedir}/va
%{_includedir}/va/*
%{_libdir}/libva.so
%{_libdir}/libva-tpi.so
%{_libdir}/libva-x11.so
%{_libdir}/pkgconfig/libva.pc
%{_libdir}/pkgconfig/libva-x11.pc

%changelog
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

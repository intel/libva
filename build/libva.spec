Name:       libva
Version:    %{_version}
Release:    el7
Summary:    Intel libva
License:    MIT
Source0:    %{_sourcefile}

%package devel
Group: Development/Libraries
Summary: Development files for libva

%description devel
Development files for libva

BuildRequires: automake
BuildRequires: autoconf

%description
Intel libva

%prep

%setup

%build
./configure --prefix %{_prefix} --libdir %{_libdir}
make -j$(nproc)

%install
make install DESTDIR=%{buildroot}

%files
%{_libdir}/*.so*
%files devel
%{_includedir}/va/*.h
%{_libdir}/pkgconfig/*.pc
%{_libdir}/*.la

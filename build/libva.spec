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
./autogen.sh 
make -j12

%install
make install DESTDIR=%{buildroot}
#mkdir -p %{buildroot}/usr/bin/
#install -m 755 hello-world.sh %{buildroot}/usr/bin/hello-world.sh

%files
/usr/local/lib/*.so*
%files devel
/usr/local/include/va/*.h
/usr/local/lib/pkgconfig/*.pc
/usr/local/lib/*.la

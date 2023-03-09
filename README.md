# Libva Project

Libva is an implementation for VA-API (Video Acceleration API)

VA-API is an open-source library and API specification, which provides access to graphics hardware acceleration capabilities for video processing. It consists of a main library and driver-specific acceleration backends for each supported hardware vendor.

If you would like to contribute to libva, check our [Contributing guide](https://github.com/intel/libva/blob/master/CONTRIBUTING.md).

We also recommend taking a look at the ['janitorial' bugs](https://github.com/intel/libva/issues?q=is%3Aopen+is%3Aissue+label%3AJanitorial) in our list of open issues as these bugs can be solved without an extensive knowledge of libva.

We would love to help you start contributing!

Doxygen files are regularly updated through Github Pages and can be accessed directly through [github pages libva](http://intel.github.io/libva/)

The libva development team can be reached via github issues.


# Build and Install Libva
*This build documentation was tested under clear Ubuntu Server 18.04 (with gcc-7.3.0, gcc-8.1.0 and clang-6.0 compilers) but it should work on another OS distributions with various versions of gcc and clang.*
## Install all required common packages: 
```
sudo apt-get install git cmake pkg-config meson libdrm-dev automake libtool
```

Take latest libva version:
```
git clone https://github.com/intel/libva.git
cd libva
```

## Build with autogen and Meson

When you install the libva from OSV package repositories, different OSV distro use different default location for libva. Basically, Debian/Ubuntu family install libva to /usr/lib/x86_64-linux-gnu and rpm family like Fedora/CentOS/SUSE/RHEL install libva to /usr/lib64. For Other media component default location, you could refer to [Install from OSV package](https://github.com/intel/media-driver/wiki/Install-from-OSV-package))

Without prefix setting, libva will be install to /usr/local/lib as default. If you use other path as installation target folder or no prefix, you have to add the folder to your environment variable or use LD_LIBRARY_PATH to specify the location, like LD_LIBRARY_PATH=/usr/local/lib if no prefix.

If you intent to overwrite system default libva, you could use same OSV distro prefix, then system libva will be replaced and also your new installed libva version will be overwrited when you upgrade it from OSV distro package repository. 

For debian family, you could use autogen
```
./autogen.sh --prefix=/usr --libdir=/usr/lib/x86_64-linux-gnu
make
sudo make install
```
or build using Meson
```
mkdir build 
cd build 
meson .. -Dprefix=/usr -Dlibdir=/usr/lib/x86_64-linux-gnu
ninja
sudo ninja install
```

For rpm family, you could use autogen
```
./autogen.sh --prefix=/usr --libdir=/usr/lib64
make
sudo make install
```
or build using Meson
```
mkdir build 
cd build 
meson .. -Dprefix=/usr -Dlibdir=/usr/lib64
ninja
sudo ninja install
```

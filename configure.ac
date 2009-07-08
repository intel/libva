# Copyright (c) 2007 Intel Corporation. All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sub license, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
# 
# The above copyright notice and this permission notice (including the
# next paragraph) shall be included in all copies or substantial portions
# of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
# IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
# ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# libva package version number, (as distinct from shared library version)
m4_define([libva_major_version], [0])
m4_define([libva_minor_version], [30])
m4_define([libva_micro_version], [4])

m4_define([libva_version],
          [libva_major_version.libva_minor_version.libva_micro_version])

# if the library source code has changed, increment revision
m4_define([libva_lt_revision], [0])
# if any interface was added/removed/changed, then inc current, reset revision
m4_define([libva_lt_current], [1])
# if any interface was added since last public release, then increment age
# if any interface was removed since last public release, then set age to 0
m4_define([libva_lt_age], [0])

AC_PREREQ(2.57)
AC_INIT([libva], [libva_version], [waldo.bastian@intel.com], libva)
AC_CONFIG_SRCDIR([Makefile.am])
AM_INIT_AUTOMAKE([dist-bzip2])

AM_CONFIG_HEADER([config.h])

LIBVA_MAJOR_VERSION=libva_major_version
LIBVA_MINOR_VERSION=libva_minor_version
LIBVA_MICRO_VERSION=libva_micro_version
LIBVA_VERSION=libva_version
AC_SUBST(LIBVA_MAJOR_VERSION)
AC_SUBST(LIBVA_MINOR_VERSION)
AC_SUBST(LIBVA_MICRO_VERSION)
AC_SUBST(LIBVA_VERSION)

LIBVA_LT_CURRENT=libva_lt_current
LIBVA_LT_REV=libva_lt_revision
LIBVA_LT_AGE=libva_lt_age
LIBVA_LT_VERSION="$LIBVA_LT_CURRENT:$LIBVA_LT_REV:$LIBVA_LT_AGE"
LIBVA_LT_LDFLAGS="-version-info $LIBVA_LT_VERSION -release $LIBVA_VERSION"
AC_SUBST(LIBVA_LT_VERSION)
AC_SUBST(LIBVA_LT_LDFLAGS)

AC_ARG_ENABLE(dummy-driver,
              [AC_HELP_STRING([--enable-dummy-driver],
                              [build dummy video driver])],
              [], [enable_dummy_driver=yes])
AM_CONDITIONAL(BUILD_DUMMY_DRIVER, test x$enable_dummy_driver = xyes)

AC_ARG_ENABLE(i965-driver,
              [AC_HELP_STRING([--enable-i965-driver],
                              [build i965 video driver])],
              [], [enable_i965_driver=yes])

AC_ARG_WITH(drivers-path,
	    [AC_HELP_STRING([--with-drivers-path=[[path]]], [drivers path])],,
	    [with_drivers_path="$libdir/va/drivers"])

LIBVA_DRIVERS_PATH="$with_drivers_path"
AC_SUBST(LIBVA_DRIVERS_PATH)

AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_CC

AC_HEADER_STDC
AC_SYS_LARGEFILE

PKG_CHECK_MODULES([X11], [x11])
PKG_CHECK_MODULES([XEXT],[xext])
PKG_CHECK_MODULES([XFIXES], [xfixes])
PKG_CHECK_MODULES([DRM], [libdrm])

AC_CHECK_PROG(gen4asm, [intel-gen4asm], yes, no)
AM_CONDITIONAL(HAVE_GEN4ASM, test x$gen4asm = xyes)

# Check for libdrm >= 2.4 (needed for i965_drv_video.so)
if test x$enable_i965_driver = xyes && ! $PKG_CONFIG --atleast-version=2.4 libdrm; then
    AC_MSG_WARN([libdrm < 2.4 found, disabling build of i965 video driver])
    enable_i965_driver=no
fi
AM_CONDITIONAL(BUILD_I965_DRIVER, test x$enable_i965_driver = xyes)

# We only need the headers, we don't link against the DRM libraries
LIBVA_CFLAGS="$DRM_CFLAGS"
AC_SUBST(LIBVA_CFLAGS)
AC_SUBST(LIBVA_LIBS)

pkgconfigdir=${libdir}/pkgconfig
AC_SUBST(pkgconfigdir)

libvabackendlib=libva.la
AC_SUBST([libvabackendlib])

AC_OUTPUT([
	Makefile
	src/Makefile
	src/x11/Makefile
	dummy_drv_video/Makefile
	i965_drv_video/Makefile
	i965_drv_video/shaders/Makefile
	i965_drv_video/shaders/mpeg2/Makefile
	i965_drv_video/shaders/mpeg2/vld/Makefile
	i965_drv_video/shaders/render/Makefile
	test/Makefile
	libva.pc
])


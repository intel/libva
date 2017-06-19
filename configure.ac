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

# VA-API version
# - increment major for any ABI change
# - increment minor for any interface change (e.g. new/modified function)
# - increment micro for any other change (new flag, new codec definition, etc.)
# - reset micro version to zero when minor version is incremented
# - reset minor version to zero when major version is incremented
m4_define([va_api_major_version], [1])
m4_define([va_api_minor_version], [0])
m4_define([va_api_micro_version], [0])

m4_define([va_api_version],
          [va_api_major_version.va_api_minor_version.va_api_micro_version])

# libva package version number, (as distinct from shared library version)
# XXX: we want the package version to remain at 1.0.x for VA-API 0.32.y
#
# - major version is automatically generated from VA-API major version
# - minor version is automatically generated from VA-API minor version
# - increment micro for any library release
# - reset micro version to zero when VA-API major or minor version is changed
m4_define([libva_major_version], [m4_eval(va_api_major_version + 1)])
m4_define([libva_minor_version], [m4_eval(va_api_minor_version)])
m4_define([libva_micro_version], [0])
m4_define([libva_pre_version],   [1])

m4_define([libva_version],
          [libva_major_version.libva_minor_version.libva_micro_version])
m4_if(libva_pre_version, [0], [], [
m4_append([libva_version], libva_pre_version, [.pre])
])

# libva library version number (generated, do not change)
# XXX: we want the SONAME to remain at libva.so.1 for VA-API major == 0
#
# The library name is generated libva.<x>.<y>.0 where
# <x> = VA-API major version + 1
# <y> = 100 * VA-API minor version + VA-API micro version
#
# For example:
# VA-API 0.32.0 generates libva.so.1.3200.0
# VA-API 0.34.1 generates libva.so.1.3401.0
# VA-API 1.2.13 generates libva.so.2.213.0
m4_define([libva_interface_bias], [m4_eval(va_api_major_version + 1)])
m4_define([libva_interface_age],  [0])
m4_define([libva_binary_age],
          [m4_eval(100 * va_api_minor_version + va_api_micro_version - libva_interface_age)])

m4_define([libva_lt_current],
          [m4_eval(100 * va_api_minor_version + va_api_micro_version + libva_interface_bias)])
m4_define([libva_lt_revision],
          [m4_eval(libva_interface_age)])
m4_define([libva_lt_age],
          [m4_eval(libva_binary_age - libva_interface_age)])

# libdrm minimun version requirement
m4_define([libdrm_version], [2.4])

# Wayland minimum version number
# 1.11.0 for wl_proxy_create_wrapper
m4_define([wayland_api_version], [1.11.0])

AC_PREREQ(2.57)
AC_INIT([libva],
        [libva_version],
        [https://github.com/01org/libva/issues/new],
        [libva],
        [https://github.com/01org/libva])

AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([dist-bzip2])

AC_CONFIG_HEADERS([config.h])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

TODAY="`LC_ALL=C date +'%a, %d %b %Y %X %z'`"
AC_SUBST(TODAY)

VA_API_MAJOR_VERSION=va_api_major_version
VA_API_MINOR_VERSION=va_api_minor_version
VA_API_MICRO_VERSION=va_api_micro_version
VA_API_VERSION=va_api_version
AC_SUBST(VA_API_MAJOR_VERSION)
AC_SUBST(VA_API_MINOR_VERSION)
AC_SUBST(VA_API_MICRO_VERSION)
AC_SUBST(VA_API_VERSION)

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
LIBVA_LT_LDFLAGS="-version-info $LIBVA_LT_VERSION"
AC_SUBST(LIBVA_LT_VERSION)
AC_SUBST(LIBVA_LT_LDFLAGS)

AC_ARG_ENABLE(docs,
    [AC_HELP_STRING([--enable-docs],
                    [build Doxygen documentation @<:@default=no@:>@])],
    [], [enable_docs="no"])

AC_ARG_ENABLE(drm,
    [AC_HELP_STRING([--enable-drm],
                    [build with VA/DRM API support @<:@default=yes@:>@])],
    [], [enable_drm="yes"])

AC_ARG_ENABLE(x11,
    [AC_HELP_STRING([--enable-x11],
                    [build with VA/X11 API support @<:@default=auto@:>@])],
    [], [enable_x11="auto"])

AC_ARG_ENABLE(glx,
    [AC_HELP_STRING([--enable-glx],
                    [build with VA/GLX API support @<:@default=auto@:>@])],
    [], [enable_glx="auto"])

AC_ARG_ENABLE([wayland],
    [AC_HELP_STRING([--enable-wayland],
                    [build with VA/Wayland API support @<:@default=auto@:>@])],
    [], [enable_wayland="auto"])

AC_ARG_ENABLE([va-messaging],
    [AC_HELP_STRING([--enable-va-messaging],
                    [build with va info and error messaging @<:@default=yes@:>@])],
    [], [enable_va_messaging="yes"])

AC_ARG_WITH(drivers-path,
    [AC_HELP_STRING([--with-drivers-path=[[path]]],
                    [drivers path])],
    [], [with_drivers_path="$libdir/dri"])

LIBVA_DRIVERS_PATH="$with_drivers_path"
AC_SUBST(LIBVA_DRIVERS_PATH)

AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_CC
AC_PROG_CXX
AM_PROG_CC_C_O
AC_PROG_SED
PKG_PROG_PKG_CONFIG

AC_HEADER_STDC
AC_SYS_LARGEFILE

# Check for Doxygen
if test "$enable_docs" = "yes"; then
    AC_CHECK_TOOL([DOXYGEN], [doxygen], [no])
    if test "$DOXYGEN" = "no"; then
       enable_docs="no"
    fi
fi
AM_CONDITIONAL(ENABLE_DOCS, test "$enable_docs" = "yes")

# Check for va messaging
if test "$enable_va_messaging" = "yes"; then
    AC_DEFINE([ENABLE_VA_MESSAGING], [1], [Defined to 1 if va messaging is needed])
fi

# Check for __attribute__((visibility()))
AC_CACHE_CHECK([whether __attribute__((visibility())) is supported],
    ac_cv_have_gnuc_visibility_attribute,
    [cat > conftest.c <<EOF
int foo __attribute__ ((visibility ("hidden"))) = 1;
int bar __attribute__ ((visibility ("protected"))) = 1;
EOF
    ac_cv_have_gnuc_visibility_attribute="no"
    if ${CC-cc} -Werror -S conftest.c -o conftest.s >/dev/null 2>&1; then
        if grep '\.hidden.*foo' conftest.s >/dev/null; then
            if grep '\.protected.*bar' conftest.s >/dev/null; then
                ac_cv_have_gnuc_visibility_attribute="yes"
            fi
        fi
    fi
    rm -f conftest.[cs]
])
if test "$ac_cv_have_gnuc_visibility_attribute" = "yes"; then
    AC_DEFINE([HAVE_GNUC_VISIBILITY_ATTRIBUTE], [1],
              [Defined to 1 if GCC visibility attribute is supported])
fi

# Check for DRM (mandatory)
LIBDRM_VERSION=libdrm_version
PKG_CHECK_MODULES([DRM], [libdrm >= $LIBDRM_VERSION])
AC_SUBST(LIBDRM_VERSION)

USE_DRM="no"
if test "$enable_drm" = "yes"; then
    USE_DRM="yes"
    AC_DEFINE([HAVE_VA_DRM], [1], [Defined to 1 if VA/DRM API is built])
fi
AM_CONDITIONAL(USE_DRM, test "$USE_DRM" = "yes")

# Check for X11
USE_X11="no"
if test "x$enable_x11" != "xno"; then
    PKG_CHECK_MODULES([X11],    [x11 xext xfixes],    [USE_X11="yes"], [:])

    if test "x$USE_X11" = "xno" -a "x$enable_x11" = "xyes"; then
       AC_MSG_ERROR([VA/X11 explicitly enabled, however $X11_PKG_ERRORS])
    fi

    if test "$USE_X11" = "yes"; then
        AC_DEFINE([HAVE_VA_X11], [1], [Defined to 1 if VA/X11 API is built])
    fi
fi
AM_CONDITIONAL(USE_X11, test "$USE_X11" = "yes")

# Check for GLX
USE_GLX="no"

if test "$USE_X11:$enable_glx" = "no:yes"; then
   AC_MSG_ERROR([VA/GLX explicitly enabled, but VA/X11 isn't built])
fi

if test "$USE_X11:$enable_glx" != "yes:no"; then
    PKG_CHECK_MODULES([GLX], [gl x11], [USE_GLX="yes"], [:])
    saved_CPPFLAGS="$CPPFLAGS"
    saved_LIBS="$LIBS"
    CPPFLAGS="$CPPFLAGS $GLX_CFLAGS"
    LIBS="$LIBS $GLX_LIBS"
    AC_CHECK_HEADERS([GL/gl.h GL/glx.h], [:], [USE_GLX="no"])
    AC_CHECK_LIB([GL], [glXCreateContext], [:] [USE_GLX="no"])
    CPPFLAGS="$saved_CPPFLAGS"
    LIBS="$saved_LIBS"

    if test "x$USE_GLX" = "xno" -a "x$enable_glx" = "xyes"; then
       AC_MSG_ERROR([VA/GLX explicitly enabled, but libGL couldn't be found])
    fi

    if test "$USE_GLX" = "yes"; then
        AC_DEFINE([HAVE_VA_GLX], [1], [Defined to 1 if VA/GLX API is built])
    fi
fi
AM_CONDITIONAL(USE_GLX, test "$USE_GLX" = "yes")

# Check for Wayland
WAYLAND_API_VERSION=wayland_api_version
AC_SUBST(WAYLAND_API_VERSION)

USE_WAYLAND="no"
if test "x$enable_wayland" != "xno"; then
    PKG_CHECK_MODULES([WAYLAND], [wayland-client >= wayland_api_version],
        [USE_WAYLAND="yes"], [:])

    if test "x$USE_WAYLAND" = "xno" -a "x$enable_wayland" = "xyes"; then
        AC_MSG_ERROR([wayland explicitly enabled, however $WAYLAND_PKG_ERRORS])
    fi

    if test "$USE_WAYLAND" = "yes"; then

        WAYLAND_PREFIX=`$PKG_CONFIG --variable=prefix wayland-client`
        AC_PATH_PROG([WAYLAND_SCANNER], [wayland-scanner],,
                     [${WAYLAND_PREFIX}/bin$PATH_SEPARATOR$PATH])
        if test "x$WAYLAND_SCANNER" = "x"; then
            AC_MSG_ERROR([wayland-scanner not found: Install it or use --disable-wayland])
        fi

        AC_DEFINE([HAVE_VA_WAYLAND], [1],
                  [Defined to 1 if VA/Wayland API is built])
    fi
fi

AM_CONDITIONAL(USE_WAYLAND, test "$USE_WAYLAND" = "yes")

pkgconfigdir=${libdir}/pkgconfig
AC_SUBST(pkgconfigdir)

# Check for builds without backend
if test "$USE_DRM:$USE_X11:$USE_WAYLAND" = "no:no:no"; then
    AC_MSG_ERROR([Please select at least one backend (DRM, X11, Wayland)])
fi

AC_OUTPUT([
    Makefile
    doc/Makefile
    pkgconfig/Makefile
    pkgconfig/libva-drm.pc
    pkgconfig/libva-glx.pc
    pkgconfig/libva-wayland.pc
    pkgconfig/libva-x11.pc
    pkgconfig/libva.pc
    va/Makefile
    va/drm/Makefile
    va/glx/Makefile
    va/va_version.h
    va/wayland/Makefile
    va/x11/Makefile
])

# Print a small summary
BACKENDS=""
AS_IF([test x$USE_DRM = xyes], [BACKENDS="$BACKENDS drm"])
AS_IF([test x$USE_X11 = xyes], [BACKENDS="$BACKENDS x11"])
AS_IF([test x$USE_GLX = xyes], [BACKENDS="$BACKENDS glx"])
AS_IF([test x$USE_WAYLAND = xyes], [BACKENDS="$BACKENDS wayland"])

echo
echo "libva - ${LIBVA_VERSION} (VA-API ${VA_API_VERSION})"
echo
echo Installation prefix .............. : $prefix
echo Default driver path .............. : $LIBVA_DRIVERS_PATH
echo Extra window systems ............. : $BACKENDS
echo Build documentation .............. : $enable_docs
echo Build with messaging ............. : $enable_va_messaging
echo

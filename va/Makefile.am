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

SUBDIRS =

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DVA_DRIVERS_PATH="\"$(LIBVA_DRIVERS_PATH)\""

LDADD = \
	$(LIBVA_LT_LDFLAGS)

libva_source_c = \
	va.c			\
	va_compat.c		\
	va_fool.c		\
	va_trace.c		\
	$(NULL)

libva_source_h = \
	va.h			\
	va_backend.h		\
	va_backend_tpi.h	\
	va_backend_vpp.h	\
	va_compat.h		\
	va_dec_hevc.h		\
	va_dec_jpeg.h		\
	va_dec_vp8.h		\
	va_dec_vp9.h		\
	va_drmcommon.h		\
	va_enc_hevc.h		\
	va_enc_h264.h		\
	va_enc_jpeg.h		\
	va_enc_vp8.h		\
	va_enc_mpeg2.h		\
	va_enc_vp9.h            \
	va_tpi.h		\
	va_version.h		\
	va_vpp.h		\
	$(NULL)

libva_source_h_priv = \
	sysdeps.h		\
	va_fool.h		\
	va_trace.h		\
	$(NULL)

libva_ldflags = \
	$(LDADD) -no-undefined	\
	-Wl,-version-script,${srcdir}/libva.syms \
	$(NULL)

lib_LTLIBRARIES				= libva-@LIBVA_LIB_VERSION@.la
libva_@LIBVA_LIB_VERSION@_includedir		= ${includedir}/va-@LIBVA_LIB_VERSION@/va
libva_@LIBVA_LIB_VERSION@_include_HEADERS	= $(libva_source_h)
noinst_HEADERS					= $(libva_source_h_priv)
libva_@LIBVA_LIB_VERSION@_la_SOURCES		= $(libva_source_c)
libva_@LIBVA_LIB_VERSION@_la_LDFLAGS		= $(libva_ldflags)
libva_@LIBVA_LIB_VERSION@_la_DEPENDENCIES	= libva.syms
libva_@LIBVA_LIB_VERSION@_la_LIBADD		= $(LIBVA_LIBS) -ldl

lib_LTLIBRARIES				+= libva-tpi-@LIBVA_LIB_VERSION@.la
libva_tpi_@LIBVA_LIB_VERSION@_la_SOURCES	= va_tpi.c
libva_tpi_@LIBVA_LIB_VERSION@_la_LDFLAGS	= $(LDADD) -no-undefined
libva_tpi_@LIBVA_LIB_VERSION@_la_DEPENDENCIES	= libva-@LIBVA_LIB_VERSION@.la
libva_tpi_@LIBVA_LIB_VERSION@_la_LIBADD		= libva-@LIBVA_LIB_VERSION@.la -ldl

if USE_DRM
SUBDIRS				+= drm
lib_LTLIBRARIES			+= libva-drm-@LIBVA_LIB_VERSION@.la
libva_drm_@LIBVA_LIB_VERSION@_la_SOURCES	=
libva_drm_@LIBVA_LIB_VERSION@_la_LDFLAGS	= $(LDADD)
libva_drm_@LIBVA_LIB_VERSION@_la_DEPENDENCIES	= libva-@LIBVA_LIB_VERSION@.la drm/libva_drm.la
libva_drm_@LIBVA_LIB_VERSION@_la_LIBADD		= libva-@LIBVA_LIB_VERSION@.la drm/libva_drm.la \
	$(LIBVA_LIBS) $(DRM_LIBS) -ldl
endif

if USE_X11
SUBDIRS				+= x11
lib_LTLIBRARIES			+= libva-x11-@LIBVA_LIB_VERSION@.la
libva_source_h					+= va_x11.h
libva_x11_@LIBVA_LIB_VERSION@_la_SOURCES	=
libva_x11_@LIBVA_LIB_VERSION@_la_LDFLAGS	= $(LDADD)
libva_x11_@LIBVA_LIB_VERSION@_la_DEPENDENCIES	= libva-@LIBVA_LIB_VERSION@.la x11/libva_x11.la
libva_x11_@LIBVA_LIB_VERSION@_la_LIBADD		= libva-@LIBVA_LIB_VERSION@.la x11/libva_x11.la \
	$(LIBVA_LIBS) $(X11_LIBS) $(XEXT_LIBS) $(XFIXES_LIBS) $(DRM_LIBS) -ldl
endif

if USE_GLX
SUBDIRS				+= glx
lib_LTLIBRARIES			+= libva-glx-@LIBVA_LIB_VERSION@.la
libva_glx_@LIBVA_LIB_VERSION@_la_SOURCES	=
libva_glx_@LIBVA_LIB_VERSION@_la_LDFLAGS	= $(LDADD)
libva_glx_@LIBVA_LIB_VERSION@_la_DEPENDENCIES	= libva-@LIBVA_LIB_VERSION@.la glx/libva_glx.la libva-x11-@LIBVA_LIB_VERSION@.la
libva_glx_@LIBVA_LIB_VERSION@_la_LIBADD		= libva-@LIBVA_LIB_VERSION@.la glx/libva_glx.la libva-x11-@LIBVA_LIB_VERSION@.la \
	$(GLX_LIBS) -ldl
endif

if USE_EGL
SUBDIRS				+= egl
lib_LTLIBRARIES			+= libva-egl-@LIBVA_LIB_VERSION@.la
libva_egl_@LIBVA_LIB_VERSION@_la_SOURCES	=
libva_egl_@LIBVA_LIB_VERSION@_la_LDFLAGS	= $(LDADD)
libva_egl_@LIBVA_LIB_VERSION@_la_DEPENDENCIES	= libva-@LIBVA_LIB_VERSION@.la egl/libva_egl.la
libva_egl_@LIBVA_LIB_VERSION@_la_LIBADD		= libva-@LIBVA_LIB_VERSION@.la egl/libva_egl.la \
	$(EGL_LIBS) -ldl
endif

if USE_WAYLAND
SUBDIRS				+= wayland
lib_LTLIBRARIES			+= libva-wayland-@LIBVA_LIB_VERSION@.la
libva_wayland_@LIBVA_LIB_VERSION@_la_SOURCES		=
libva_wayland_@LIBVA_LIB_VERSION@_la_LDFLAGS		= $(LDADD)
libva_wayland_@LIBVA_LIB_VERSION@_la_DEPENDENCIES	= libva-@LIBVA_LIB_VERSION@.la wayland/libva_wayland.la
libva_wayland_@LIBVA_LIB_VERSION@_la_LIBADD		= libva-@LIBVA_LIB_VERSION@.la wayland/libva_wayland.la \
	$(WAYLAND_LIBS) $(DRM_LIBS) -ldl
endif

DIST_SUBDIRS = x11 glx egl drm wayland

DISTCLEANFILES = \
	va_version.h		\
	$(NULL)

EXTRA_DIST = \
	libva.syms		\
	va_version.h.in		\
	$(NULL)

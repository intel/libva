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

INCLUDES = \
	$(LIBVA_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/va/x11 \
	-DVA_DRIVERS_PATH="\"$(LIBVA_DRIVERS_PATH)\""

LDADD = \
	$(LIBVA_LT_LDFLAGS)

libva_source_c = \
	va.c			\
	va_fool.c		\
	va_trace.c		\
	$(NULL)

libva_source_h = \
	va.h			\
	va_backend.h		\
	va_backend_tpi.h	\
	va_backend_vpp.h	\
	va_dummy.h		\
	va_tpi.h		\
	va_version.h		\
	va_vpp.h		\
	va_x11.h		\
	$(NULL)

libva_source_h_priv = \
	sysdeps.h		\
	va_fool.h		\
	va_trace.h		\
	$(NULL)

lib_LTLIBRARIES			= libva.la
libvaincludedir			= ${includedir}/va
libvainclude_HEADERS		= $(libva_source_h)
noinst_HEADERS			= $(libva_source_h_priv)
libva_la_SOURCES		= $(libva_source_c)
libva_la_LDFLAGS		= $(LDADD) -no-undefined
libva_la_LIBADD			= $(LIBVA_LIBS) -ldl

lib_LTLIBRARIES			+= libva-tpi.la
libva_tpi_la_SOURCES		= va_tpi.c
libva_tpi_la_LDFLAGS		= $(LDADD) -no-undefined
libva_tpi_la_DEPENDENCIES	= $(libvacorelib) 
libva_tpi_la_LIBADD		= $(libvacorelib) -ldl

SUBDIRS				+= x11
lib_LTLIBRARIES			+= libva-x11.la
libva_x11_la_SOURCES		= 
libva_x11_la_LDFLAGS		= $(LDADD)
libva_x11_la_DEPENDENCIES	= $(libvacorelib) x11/libva_x11.la
libva_x11_la_LIBADD		= $(libvacorelib) x11/libva_x11.la \
	$(LIBVA_LIBS) $(X11_LIBS) $(XEXT_LIBS) $(DRM_LIBS) $(XFIXES_LIBS) -ldl

if USE_GLX
SUBDIRS				+= glx
lib_LTLIBRARIES			+= libva-glx.la
libva_glx_la_SOURCES		=
libva_glx_la_LDFLAGS		= $(LDADD)
libva_glx_la_DEPENDENCIES	= $(libvacorelib) glx/libva_glx.la libva-x11.la
libva_glx_la_LIBADD		= $(libvacorelib) glx/libva_glx.la libva-x11.la \
	$(GL_DEPS_LIBS) -ldl
endif

if USE_EGL
SUBDIRS				+= egl
lib_LTLIBRARIES			+= libva-egl.la
libva_egl_la_SOURCES		=
libva_egl_la_LDFLAGS		= $(LDADD)
libva_egl_la_DEPENDENCIES	= $(libvacorelib) egl/libva_egl.la libva-x11.la
libva_egl_la_LIBADD		= $(libvacorelib) egl/libva_egl.la libva-x11.la \
	$(GL_DEPS_LIBS) -ldl
endif

if BUILD_DUMMY_BACKEND
SUBDIRS				+= dummy
lib_LTLIBRARIES			+= libva-dummy.la
libva_dummy_la_SOURCES		= 
libva_dummy_la_LDFLAGS		= $(LDADD)
libva_dummy_la_DEPENDENCIES	= $(libvacorelib) dummy/libva_dummy.la
libva_dummy_la_LIBADD		= $(libvacorelib) dummy/libva_dummy.la \
	$(LIBVA_LIBS) $(DRM_LIBS)
endif

DIST_SUBDIRS = x11 glx egl dummy

DISTCLEANFILES = \
	va_version.h		\
	$(NULL)

EXTRA_DIST = \
	va_version.h.in		\
	$(NULL)

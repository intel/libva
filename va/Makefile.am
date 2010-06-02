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

INCLUDES = \
	$(LIBVA_CFLAGS) -I$(top_srcdir)/va/x11 \
	-DVA_DRIVERS_PATH="\"$(LIBVA_DRIVERS_PATH)\""

LDADD = \
	$(LIBVA_LT_LDFLAGS)

libva_la_SOURCES = va.c va_trace.c
libva_ladir = $(libdir)
libva_la_LDFLAGS = $(LDADD) -no-undefined
libva_la_LIBADD = $(LIBVA_LIBS) -ldl

libva_tpi_la_SOURCES = va_tpi.c
libva_tpi_ladir = $(libdir)
libva_tpi_la_LDFLAGS = $(LDADD) -no-undefined
libva_tpi_la_LIBADD = $(libvacorelib) -ldl
libva_tpi_la_DEPENDENCIES = $(libvacorelib) 


libva_x11_backend      = libva-x11.la
libva_x11_backenddir   = x11

libva_x11_la_SOURCES = 
libva_x11_la_LIBADD  = $(libvacorelib) x11/libva_x11.la $(LIBVA_LIBS) $(X11_LIBS) $(XEXT_LIBS) $(DRM_LIBS) $(XFIXES_LIBS)
libva_x11_la_LDFLAGS = $(LDADD)
libva_x11_la_DEPENDENCIES = $(libvacorelib) x11/libva_x11.la

if BUILD_DUMMY_BACKEND
libva_dummy_backend = libva-dummy.la
libva_dummy_backenddir = dummy

libva_dummy_la_SOURCES = 
libva_dummy_la_LIBADD  = $(libvacorelib) dummy/libva_dummy.la $(LIBVA_LIBS) $(DRM_LIBS)
libva_dummy_la_LDFLAGS = $(LDADD)
libva_dummy_la_DEPENDENCIES = $(libvacorelib) dummy/libva_dummy.la
else
libva_dummy_backend =
libva_dummy_backenddir =
endif

if USE_GLX
libva_glx_backend      = libva-glx.la
libva_glx_backenddir   = glx

libva_glx_la_SOURCES =
libva_glx_la_LIBADD  = $(libvacorelib) glx/libva_glx.la libva-x11.la $(GL_DEPS_LIBS) -ldl
libva_glx_la_LDFLAGS = $(LDADD)
libva_glx_la_DEPENDENCIES = $(libvacorelib) glx/libva_glx.la libva-x11.la

else
libva_glx_backend      =
libva_glx_backenddir   =
endif

lib_LTLIBRARIES = libva.la libva-tpi.la $(libva_x11_backend) $(libva_dummy_backend) $(libva_glx_backend)

DIST_SUBDIRS = x11 glx dummy

SUBDIRS = $(libva_x11_backenddir) $(libva_dummy_backenddir) $(libva_glx_backenddir)

libvaincludedir = ${includedir}/va
libvainclude_HEADERS = va.h va_tpi.h va_x11.h va_backend.h va_backend_tpi.h va_dummy.h va_version.h

DISTCLEANFILES = \
	va_version.h

EXTRA_DIST = \
	va_version.h.in

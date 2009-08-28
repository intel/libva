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
	$(LIBVA_CFLAGS) -I$(top_srcdir)/src/x11 \
	-DIN_LIBVA \
	-DVA_DRIVERS_PATH="\"$(LIBVA_DRIVERS_PATH)\""

LDADD = \
	$(LIBVA_LT_LDFLAGS)

lib_LTLIBRARIES = \
	libva.la \
	libva-x11.la

libva_ladir = $(libdir)
libva_la_LDFLAGS = $(LDADD) -no-undefined
libva_la_LIBADD = $(LIBVA_LIBS) -ldl

libva_x11_la_SOURCES = 
libva_x11_la_LIBADD  = $(libvacorelib) x11/libva_x11.la $(LIBVA_LIBS) $(X11_LIBS) $(XEXT_LIBS) $(DRM_LIBS) $(XFIXES_LIBS)
libva_x11_la_LDFLAGS = $(LDADD)
libva_x11_la_DEPENDENCIES = $(libvacorelib)

SUBDIRS = x11

libva_la_SOURCES = va.c

libvaincludedir = ${includedir}/va
libvainclude_HEADERS = va.h va_backend.h va_version.h

DISTCLEANFILES = \
	va_version.h

EXTRA_DIST = \
	va_version.h.in

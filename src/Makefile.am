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

libva_la_LTLIBRARIES = libva.la
libva_ladir = $(libdir)
libva_la_LDFLAGS = -version-number 0:30:4 -no-undefined
libva_la_LIBADD = $(LIBVA_LIBS) -ldl -lX11 -lXext x11/libva_x11.la -ldrm -lXfixes

nodist_libva_la_SOURCES = va_version.h
BUILT_SOURCES = va_version.h

CLEANFILES = va_version.h

va_version.h: Makefile
	echo "#define VA_BUILD_DATE \"$(shell date +'%Y%m%d') $(shell date +'1%H%M%S') \"" > va_version.h
	echo "#define VA_BUILD_GIT  \"($(shell git log  | head -n1 | cut -f2 -d' ')) \" "    >> va_version.h

SUBDIRS = x11

libva_la_SOURCES = va.c

libvaincludedir = ${includedir}/va
libvainclude_HEADERS = va.h va_backend.h

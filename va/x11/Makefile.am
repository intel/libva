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

AM_CPPFLAGS = \
	-DLINUX			\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/va	\
	$(X11_CFLAGS)		\
	$(DRM_CFLAGS)		\
	$(NULL)

source_c = \
	dri2_util.c		\
	va_dri2.c		\
	va_dricommon.c		\
	va_fglrx.c		\
	va_nvctrl.c		\
	va_x11.c		\
	$(NULL)

source_h = \
	va_dri2.h		\
	va_dricommon.h		\
	$(NULL)

source_h_priv = \
	va_dri2str.h		\
	va_dri2tokens.h		\
	va_fglrx.h		\
	va_nvctrl.h		\
	$(NULL)

noinst_LTLIBRARIES		= libva_x11.la	
libva_x11includedir		= ${includedir}/va-@LIBVA_LIB_VERSION@/va
libva_x11include_HEADERS	= $(source_h)
libva_x11_la_SOURCES		= $(source_c)
noinst_HEADERS			= $(source_h_priv)

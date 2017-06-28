# Copyright (C) 2012 Intel Corporation. All Rights Reserved.
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
	$(DRM_CFLAGS)		\
	$(NULL)

source_c = \
	va_drm.c		\
	va_drm_auth.c		\
	va_drm_utils.c		\
	$(NULL)

source_h = \
	va_drm.h		\
	$(NULL)

source_h_priv = \
	va_drm_auth.h		\
	va_drm_auth_x11.h	\
	va_drm_utils.h		\
	$(NULL)

if USE_X11
source_c			+= va_drm_auth_x11.c
AM_CPPFLAGS			+= $(X11_CFLAGS)
endif

noinst_LTLIBRARIES		= libva_drm.la
libva_drmincludedir		= ${includedir}/va-@LIBVA_LIB_VERSION@/va
libva_drminclude_HEADERS	= $(source_h)
libva_drm_la_SOURCES		= $(source_c)
noinst_HEADERS			= $(source_h_priv)

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in

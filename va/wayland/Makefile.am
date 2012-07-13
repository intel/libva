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

SUBDIRS = protocol

INCLUDES = \
	-DLINUX			\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/va	\
	$(WAYLAND_CFLAGS)	\
	$(DRM_CFLAGS)		\
	$(NULL)

source_c = \
	va_wayland.c		\
	va_wayland_drm.c	\
	va_wayland_emgd.c	\
	$(top_srcdir)/va/drm/va_drm_utils.c \
	$(NULL)

source_h = \
	va_backend_wayland.h	\
	va_wayland.h		\
	$(NULL)

source_h_priv = \
	va_wayland_drm.h	\
	va_wayland_emgd.h	\
	va_wayland_private.h	\
	$(NULL)

protocol_source_h = \
	wayland-drm-client-protocol.h	\
	$(NULL)

noinst_LTLIBRARIES		= libva_wayland.la
libva_waylandincludedir		= ${includedir}/va
libva_waylandinclude_HEADERS	= $(source_h)
libva_wayland_la_SOURCES	= $(source_c) $(protocol_source_h)
noinst_HEADERS			= $(source_h_priv)

# Wayland protocol
va_wayland_drm.c: $(protocol_source_h)
@wayland_scanner_rules@

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in

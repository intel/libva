# Copyright (C) 2009 Splitted-Desktop Systems. All Rights Reserved.
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
	-DLINUX			\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/va	\
	-I$(top_srcdir)/va/x11	\
	$(X11_CFLAGS)		\
	$(GLX_CFLAGS)		\
	$(NULL)

source_c = \
	va_glx.c		\
	va_glx_impl.c		\
	$(NULL)

source_h = \
	va_backend_glx.h	\
	va_glx.h		\
	$(NULL)

source_h_priv = \
	va_glx_impl.h		\
	va_glx_private.h	\
	$(NULL)

noinst_LTLIBRARIES		 = libva_glx.la
libva_glxincludedir		 = ${includedir}/va
libva_glxinclude_HEADERS	 = $(source_h)
libva_glx_la_SOURCES		 = $(source_c)
noinst_HEADERS			 = $(source_h_priv)

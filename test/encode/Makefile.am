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

bin_PROGRAMS = avcenc mpeg2vaenc h264encode

INCLUDES = \
       -Wall                           \
       -I$(top_srcdir)                 \
       -I$(top_srcdir)/va              \
       $(NULL)

h264encode_SOURCES	= h264encode.c
h264encode_CFLAGS	= -I$(top_srcdir)/test/common -g
h264encode_LDADD	= \
	$(top_builddir)/va/libva.la \
	$(top_builddir)/test/common/libva-display.la \
	-lpthread -lm

avcenc_SOURCES		= avcenc.c
avcenc_CFLAGS		= -I$(top_srcdir)/test/common -g
avcenc_LDADD		= \
	$(top_builddir)/va/libva.la \
	$(top_builddir)/test/common/libva-display.la \
	-lpthread

mpeg2vaenc_SOURCES	= mpeg2vaenc.c
mpeg2vaenc_CFLAGS	= -I$(top_srcdir)/test/common
mpeg2vaenc_LDADD	= \
	$(top_builddir)/va/libva.la \
	$(top_builddir)/test/common/libva-display.la \
	-lpthread

valgrind:	$(bin_PROGRAMS)
	for a in $(bin_PROGRAMS); do \
		valgrind --leak-check=full --show-reachable=yes .libs/$$a; \
	done

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

SUBDIRS = shaders

AM_CFLAGS = -Wall -I$(top_srcdir) -I$(top_srcdir)/va -I$(top_srcdir)/va/x11 -DPTHREADS @DRM_CFLAGS@

i965_drv_video_la_LTLIBRARIES = i965_drv_video.la
i965_drv_video_ladir = @LIBVA_DRIVERS_PATH@
i965_drv_video_la_LDFLAGS = -module -avoid-version -no-undefined -Wl,--no-undefined @DRM_LIBS@ -ldrm_intel
i965_drv_video_la_LIBADD = ../va/libva-x11.la -lpthread

i965_drv_video_la_SOURCES =	\
	object_heap.c		\
	intel_batchbuffer.c	\
	intel_batchbuffer_dump.c\
	intel_memman.c		\
	intel_driver.c		\
	i965_media.c		\
	i965_media_mpeg2.c	\
	i965_media_h264.c	\
	i965_render.c		\
	i965_drv_video.c	\
	i965_avc_bsd.c		\
	i965_avc_hw_scoreboard.c\
	i965_avc_ildb.c		\
	i965_post_processing.c	\
	gen6_mfd.c		\
	i965_encoder.c		\
	gen6_vme.c		\
	gen6_mfc.c		\
	gen7_mfd.c

noinst_HEADERS =                \
	object_heap.h           \
	intel_batchbuffer.h     \
	intel_batchbuffer_dump.h\
	intel_memman.h          \
	intel_driver.h          \
	i965_media.h            \
	i965_media_mpeg2.h      \
	i965_media_h264.h	\
	i965_render.h           \
	i965_drv_video.h        \
	i965_defines.h          \
	i965_structs.h		\
	i965_avc_bsd.h		\
	i965_avc_hw_scoreboard.h\
	i965_avc_ildb.h		\
	i965_post_processing.h	\
	gen6_mfd.h		\
	i965_encoder.h		\
	gen6_vme.h		\
	gen6_mfc.h		\
	intel_compiler.h	\
	i965_mutext.h		\
	gen7_mfd.h

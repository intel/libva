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

pcfiles		 = libva-@LIBVA_LIB_VERSION@.pc
pcfiles		+= libva-tpi-@LIBVA_LIB_VERSION@.pc
if USE_DRM
pcfiles		+= libva-drm-@LIBVA_LIB_VERSION@.pc
endif
if USE_X11
pcfiles		+= libva-x11-@LIBVA_LIB_VERSION@.pc
endif
if USE_GLX
pcfiles		+= libva-glx-@LIBVA_LIB_VERSION@.pc
endif
if USE_EGL
pcfiles		+= libva-egl-@LIBVA_LIB_VERSION@.pc
endif
if USE_WAYLAND
pcfiles		+= libva-wayland-@LIBVA_LIB_VERSION@.pc
endif

all_pcfiles_in	 = libva.pc.in
all_pcfiles_in	+= libva-tpi.pc.in
all_pcfiles_in	+= libva-drm.pc.in
all_pcfiles_in	+= libva-x11.pc.in
all_pcfiles_in	+= libva-glx.pc.in
all_pcfiles_in	+= libva-egl.pc.in
all_pcfiles_in	+= libva-wayland.pc.in

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = $(pcfiles)

EXTRA_DIST = $(all_pcfiles_in)

DISTCLEANFILES = $(pcfiles)

### how to generate pc files
%-@LIBVA_LIB_VERSION@.pc: %.pc
	@cp $< $@

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in

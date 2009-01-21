# INTEL CONFIDENTIAL
# Copyright 2007 Intel Corporation. All Rights Reserved.
#
# The source code contained or described herein and all documents related to
# the source code ("Material") are owned by Intel Corporation or its suppliers
# or licensors. Title to the Material remains with Intel Corporation or its
# suppliers and licensors. The Material may contain trade secrets and
# proprietary and confidential information of Intel Corporation and its
# suppliers and licensors, and is protected by worldwide copyright and trade
# secret laws and treaty provisions. No part of the Material may be used,
# copied, reproduced, modified, published, uploaded, posted, transmitted,
# distributed, or disclosed in any way without Intel's prior express written
# permission. 
# 
# No license under any patent, copyright, trade secret or other intellectual
# property right is granted to or conferred upon you by disclosure or delivery
# of the Materials, either expressly, by implication, inducement, estoppel or
# otherwise. Any license under such intellectual property rights must be
# express and approved by Intel in writing.

AM_CFLAGS = -DLINUX -I$(top_srcdir)/src $(DRM_CFLAGS)

noinst_LTLIBRARIES = libva_X11.la	

libva_X11includedir =  ${includedir}/X11
libva_X11include_HEADERS = va_x11.h va_dri.h

libva_X11_la_SOURCES = va_x11.c va_dri.c

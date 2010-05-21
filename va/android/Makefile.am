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

AM_CFLAGS = -DLINUX -I$(top_srcdir)/va -I$(top_srcdir)/va/x11 $(DRM_CFLAGS)

noinst_LTLIBRARIES = libva_dummy.la	

libva_dummy_la_LIBADD = $(LIBVA_LIBS) -ldl -ludev

libva_dummyincludedir = ${includedir}/va

libva_dummy_la_SOURCES = va_dummy.c drmtest.c


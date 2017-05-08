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

# For libva
# =====================================================

LOCAL_PATH:= $(call my-dir)

LIBVA_DRIVERS_PATH_32 := /vendor/lib/dri
LIBVA_DRIVERS_PATH_64 := /vendor/lib64/dri

include $(CLEAR_VARS)

#LIBVA_MINOR_VERSION := 31
#LIBVA_MAJOR_VERSION := 0 

LOCAL_SRC_FILES := \
	va.c \
	va_trace.c \
	va_fool.c

LOCAL_CFLAGS_32 += \
	-DVA_DRIVERS_PATH="\"$(LIBVA_DRIVERS_PATH_32)\"" \

LOCAL_CFLAGS_64 += \
	-DVA_DRIVERS_PATH="\"$(LIBVA_DRIVERS_PATH_64)\"" \

LOCAL_CFLAGS := \
	$(if $(filter user,$(TARGET_BUILD_VARIANT)),,-DENABLE_VA_MESSAGING) \
	-DLOG_TAG=\"libva\"

LOCAL_C_INCLUDES := $(LOCAL_PATH)/..

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libva
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_PROPRIETARY_MODULE := true

LOCAL_SHARED_LIBRARIES := libdl libdrm libcutils liblog

intermediates := $(call local-generated-sources-dir)

LOCAL_EXPORT_C_INCLUDE_DIRS := \
	$(intermediates) \
	$(LOCAL_C_INCLUDES)

GEN := $(intermediates)/va/va_version.h
$(GEN): SCRIPT := $(LOCAL_PATH)/../build/gen_version.sh
$(GEN): PRIVATE_CUSTOM_TOOL = sh $(SCRIPT) $(<D)/.. $< > $@
$(GEN): $(intermediates)/va/%.h : $(LOCAL_PATH)/%.h.in $(LOCAL_PATH)/../configure.ac
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN) 

include $(BUILD_SHARED_LIBRARY)

# For libva-android
# =====================================================

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	android/va_android.cpp \
	drm/va_drm_utils.c

LOCAL_CFLAGS += \
	-DLOG_TAG=\"libva-android\"

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/drm

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libva-android
LOCAL_PROPRIETARY_MODULE := true

LOCAL_SHARED_LIBRARIES := libva libdrm

include $(BUILD_SHARED_LIBRARY)

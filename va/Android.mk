# For libva_android
# =====================================================

LOCAL_PATH:= $(call my-dir)

LIBVA_MINOR_VERSION := 31
LIBVA_MAJOR_VERSION := 0 

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
   va.c \
   va_trace.c \
   android/va_android.cpp


LOCAL_CFLAGS += -DHAVE_CONFIG_H \
       -DANDROID \

LOCAL_C_INCLUDES += \
   $(TARGET_OUT_HEADERS)/libva \
   $(LOCAL_PATH)/x11 

LOCAL_CXX := g++

LOCAL_COPY_HEADERS_TO := libva/va

LOCAL_COPY_HEADERS := \
   va.h	\
   va_backend.h \
   va_version.h.in \
   x11/va_dricommon.h \
   va_android.h		

LOCAL_MODULE := libva_android

LOCAL_SHARED_LIBRARIES := libdl libdrm libcutils

include $(BUILD_SHARED_LIBRARY)


# For libva_android_tpi
# =====================================================

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
   va_tpi.c \

LOCAL_C_INCLUDES += \
   $(TARGET_OUT_HEADERS)/libva \

LOCAL_COPY_HEADERS_TO := libva/va

LOCAL_COPY_HEADERS := \
   va.h \
   va_backend.h \
   va_backend_tpi.h


LOCAL_MODULE := libva_android_tpi

include $(BUILD_SHARED_LIBRARY)

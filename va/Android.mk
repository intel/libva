LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
   android/va_android.c 

LOCAL_CFLAGS += -DHAVE_CONFIG_H \
       -DIN_LIBVA \

LOCAL_C_INCLUDES += \
   $(TOPDIR)kernel/include \
   $(TARGET_OUT_HEADERS)/libva \
   $(TOPDIR)kernel/include/drm 

LOCAL_CC := g++

LOCAL_COPY_HEADERS_TO := libva/va

LOCAL_COPY_HEADERS := \
   va.h	\
   va_backend.h \
   va_version.h.in \
   va_android.h		

LOCAL_MODULE := libva_android

LOCAL_SHARED_LIBRARIES := libdl libdrm libcutils

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))

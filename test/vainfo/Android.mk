# For vainfo
# =====================================================

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	vainfo.c		\
	../common/va_display.c	\
	../common/va_display_android.cpp

LOCAL_CFLAGS += \
  -DANDROID

LOCAL_C_INCLUDES += \
  $(TARGET_OUT_HEADERS)/libva

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := vainfo

LOCAL_SHARED_LIBRARIES := libva-android libva libdl libdrm libcutils

include $(BUILD_EXECUTABLE)


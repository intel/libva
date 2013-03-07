# For test_01
# =====================================================

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	mpeg2vldemo.cpp		\
	../common/va_display.c	\
	../common/va_display_android.cpp

LOCAL_CFLAGS += \
    -DANDROID

LOCAL_C_INCLUDES += \
  $(LOCAL_PATH)/../../va \
  $(LOCAL_PATH)/../common \
  $(TARGET_OUT_HEADERS)/libva

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE :=	mpeg2vldemo

LOCAL_SHARED_LIBRARIES := libva libva-android libdl libdrm libcutils libutils libgui

include $(BUILD_EXECUTABLE)


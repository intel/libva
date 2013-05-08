# For test_01
# =====================================================

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  ../common/va_display.c \
  ../common/va_display_android.cpp \
  h264encode.c

LOCAL_CFLAGS += \
    -DANDROID

LOCAL_C_INCLUDES += \
  $(LOCAL_PATH)/../../va \
  $(LOCAL_PATH)/../common \
  $(TARGET_OUT_HEADERS)/libva

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE :=	h264encode

LOCAL_SHARED_LIBRARIES := libva-android libva libdl libdrm  libcutils libutils libgui libm

include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	../common/va_display.c			\
	../common/va_display_android.cpp	\
	avcenc.c

LOCAL_CFLAGS += \
    -DANDROID

LOCAL_C_INCLUDES += \
  $(LOCAL_PATH)/../../va \
  $(LOCAL_PATH)/../common \
  $(TARGET_OUT_HEADERS)/libva

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE :=	avcenc

LOCAL_SHARED_LIBRARIES := libva-android libva libdl libdrm libcutils libutils libgui

include $(BUILD_EXECUTABLE)


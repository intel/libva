# For putsurface
# =====================================================

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  putsurface_android.cpp
  #putsurface_x11.c

LOCAL_CFLAGS += \
    -DANDROID -Wno-unused-parameter

LOCAL_C_INCLUDES += \
  $(TARGET_OUT_HEADERS)/libva

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := putsurface

LOCAL_SHARED_LIBRARIES := libva-android libva libdl libdrm libcutils libutils libgui

include $(BUILD_EXECUTABLE)


# For putsurface
# =====================================================

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  putsurface-android.cpp

LOCAL_CFLAGS += \
    -DANDROID  

LOCAL_C_INCLUDES += \
  $(TARGET_OUT_HEADERS)/libva

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := putsurface

LOCAL_SHARED_LIBRARIES := libva-android libva libdl libdrm libcutils libutils libui libsurfaceflinger_client

include $(BUILD_EXECUTABLE)


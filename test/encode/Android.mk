# For test_01
# =====================================================

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  h264encode_android.cpp	\
  #h264encode_x11.c	\

LOCAL_CFLAGS += \
    -DANDROID  

LOCAL_C_INCLUDES += \
  $(TARGET_OUT_HEADERS)/libva	\
  $(TOPDIR)/hardware/intel/libva/va/	\
  $(TARGET_OUT_HEADERS)/X11	

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE :=	h264encode

LOCAL_SHARED_LIBRARIES := libva-android libva libdl libdrm libcutils libutils libui libsurfaceflinger_client

include $(BUILD_EXECUTABLE)


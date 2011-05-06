# For test_01
# =====================================================

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  mpeg2vldemo.cpp	\

LOCAL_CFLAGS += \
    -DANDROID  

LOCAL_C_INCLUDES += \
  $(TARGET_OUT_HEADERS)/libva	\
  $(TOPDIR)/hardware/intel/libva/va/	\
  $(TARGET_OUT_HEADERS)/X11	

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE :=	mpeg2vldemo

LOCAL_SHARED_LIBRARIES := libva-android libva libdl libdrm libcutils libutils libui libsurfaceflinger_client

include $(BUILD_EXECUTABLE)


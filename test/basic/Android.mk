# For test_01
# =====================================================

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  test_01.c	\

LOCAL_CFLAGS += \
    -DANDROID

LOCAL_C_INCLUDES += \
  $(TARGET_OUT_HEADERS)/libva	\
  $(TOPDIR)/hardware/intel/libva/va/

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE :=	test_001

LOCAL_SHARED_LIBRARIES := libva-android libva libdl libdrm libcutils libutils libui libsurfaceflinger

include $(BUILD_EXECUTABLE)

# For test_02
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  test_02.c	\

LOCAL_CFLAGS += \
    -DANDROID

LOCAL_C_INCLUDES += \
  $(TARGET_OUT_HEADERS)/libva	\
  $(TOPDIR)/hardware/intel/libva/va/

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE :=	test_02_android

LOCAL_SHARED_LIBRARIES := libva-android libva libdl libdrm libcutils libutils libui libsurfaceflinger

include $(BUILD_EXECUTABLE)

# For test_03
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  test_03.c	\

LOCAL_CFLAGS += \
    -DANDROID

LOCAL_C_INCLUDES += \
  $(TARGET_OUT_HEADERS)/libva	\
  $(TOPDIR)/hardware/intel/libva/va/

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE :=	test_03_android

LOCAL_SHARED_LIBRARIES := libva-android libva libdl libdrm libcutils libutils libui libsurfaceflinger

include $(BUILD_EXECUTABLE)

# For test_04g
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  test_04.c	\

LOCAL_CFLAGS += \
    -DANDROID

LOCAL_C_INCLUDES += \
  $(TARGET_OUT_HEADERS)/libva	\
  $(TOPDIR)/hardware/intel/libva/va/

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE :=	test_04_android

LOCAL_SHARED_LIBRARIES := libva-android libva libdl libdrm libcutils libutils libui libsurfaceflinger

include $(BUILD_EXECUTABLE)

# For test_05
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  test_05.c	\

LOCAL_CFLAGS += \
    -DANDROID

LOCAL_C_INCLUDES += \
  $(TARGET_OUT_HEADERS)/libva	\
  $(TOPDIR)/hardware/intel/libva/va/

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE :=	test_05_android

LOCAL_SHARED_LIBRARIES := libva-android libva libdl libdrm libcutils libutils libui libsurfaceflinger

include $(BUILD_EXECUTABLE)

# For test_06
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  test_06.c	\

LOCAL_CFLAGS += \
    -DANDROID

LOCAL_C_INCLUDES += \
  $(TARGET_OUT_HEADERS)/libva	\
  $(TOPDIR)/hardware/intel/libva/va/

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE :=	test_06_android

LOCAL_SHARED_LIBRARIES := libva-android libva libdl libdrm libcutils libutils libui libsurfaceflinger

include $(BUILD_EXECUTABLE)

# For test_07
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  test_07.c	\

LOCAL_CFLAGS += \
    -DANDROID

LOCAL_C_INCLUDES += \
  $(TARGET_OUT_HEADERS)/libva	\
  $(TOPDIR)/hardware/intel/libva/va/

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE :=	test_07_android

LOCAL_SHARED_LIBRARIES := libva-android libva libdl libdrm libcutils libutils libui libsurfaceflinger

include $(BUILD_EXECUTABLE)

# For test_08
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  test_08.c	\

LOCAL_CFLAGS += \
    -DANDROID

LOCAL_C_INCLUDES += \
  $(TARGET_OUT_HEADERS)/libva	\
  $(TOPDIR)/hardware/intel/libva/va/

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE :=	test_08_android

LOCAL_SHARED_LIBRARIES := libva-android libva libdl libdrm libcutils libutils libui libsurfaceflinger

include $(BUILD_EXECUTABLE)

# For test_09
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  test_09.c	\

LOCAL_CFLAGS += \
    -DANDROID

LOCAL_C_INCLUDES += \
  $(TARGET_OUT_HEADERS)/libva	\
  $(TOPDIR)/hardware/intel/libva/va/

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE :=	test_09_android

LOCAL_SHARED_LIBRARIES := libva-android libva libdl libdrm libcutils libutils libui libsurfaceflinger

include $(BUILD_EXECUTABLE)

# For test_10
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  test_10.c	

LOCAL_CFLAGS += \
    -DANDROID

LOCAL_C_INCLUDES += \
  $(TARGET_OUT_HEADERS)/libva	\
  $(TOPDIR)/hardware/intel/libva/va/

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE :=	test_10_android

LOCAL_SHARED_LIBRARIES := libva-android libva libdl libdrm libcutils libutils libui libsurfaceflinger

include $(BUILD_EXECUTABLE)

# For test_11
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  test_11.c	

LOCAL_CFLAGS += \
    -DANDROID

LOCAL_C_INCLUDES += \
  $(TARGET_OUT_HEADERS)/libva	\
  $(TOPDIR)/hardware/intel/libva/va/

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE :=	test_11_android

LOCAL_SHARED_LIBRARIES := libva-android libva libdl libdrm libcutils libutils libui libsurfaceflinger

include $(BUILD_EXECUTABLE)


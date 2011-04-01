# For libva
# =====================================================

LOCAL_PATH:= $(call my-dir)

LIBVA_DRIVERS_PATH = /system/lib

include $(CLEAR_VARS)

#LIBVA_MINOR_VERSION := 31
#LIBVA_MAJOR_VERSION := 0 

LOCAL_SRC_FILES := \
	va.c \
	va_trace.c \
	va_fool.c \
	va_fool_getframe.c

LOCAL_CFLAGS += \
	-DANDROID \
	-DVA_DRIVERS_PATH="\"$(LIBVA_DRIVERS_PATH)\""

LOCAL_C_INCLUDES += \
	$(TARGET_OUT_HEADERS)/libva \
	$(LOCAL_PATH)/x11 \
	$(LOCAL_PATH)/..

LOCAL_COPY_HEADERS := \
	va.h \
	va_version.h \
	va_backend.h \
	va_version.h.in \
	x11/va_dricommon.h 

LOCAL_COPY_HEADERS_TO := libva/va

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libva

LOCAL_SHARED_LIBRARIES := libdl libdrm libcutils

include $(BUILD_SHARED_LIBRARY)

intermediates := $(local-intermediates-dir)
GEN := $(intermediates)/va_version.h
$(GEN): PRIVATE_GEN_VERSION := $(LOCAL_PATH)/../build/gen_version.sh
$(GEN): PRIVATE_INPUT_FILE := $(LOCAL_PATH)/va_version.h.in
$(GEN): PRIVATE_CUSTOM_TOOL = sh $(PRIVATE_GEN_VERSION) $(LOCAL_PATH)/.. $(PRIVATE_INPUT_FILE) > $@
$(GEN): $(LOCAL_PATH)/va_version.h
	$(transform-generated-source)

LOCAL_GENERATED_SOURCES += $(GEN) 

# For libva-android
# =====================================================

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	android/va_android.cpp

LOCAL_CFLAGS += \
	-DANDROID 

LOCAL_C_INCLUDES += \
	$(TARGET_OUT_HEADERS)/libva \
	$(LOCAL_PATH)/x11

LOCAL_COPY_HEADERS_TO := libva/va

LOCAL_COPY_HEADERS := va_android.h		

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libva-android

LOCAL_SHARED_LIBRARIES := libva

include $(BUILD_SHARED_LIBRARY)


# For libva-egl
# =====================================================

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	egl/va_egl.c

LOCAL_CFLAGS += \
	-DANDROID

LOCAL_C_INCLUDES += \
	$(TARGET_OUT_HEADERS)/libva \
	$(LOCAL_PATH)/x11

LOCAL_COPY_HEADERS_TO := libva/va

LOCAL_COPY_HEADERS := egl/va_egl.h egl/va_backend_egl.h

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libva-egl

LOCAL_SHARED_LIBRARIES := libva

include $(BUILD_SHARED_LIBRARY)


# For libva-tpi
# =====================================================

include $(CLEAR_VARS)

LOCAL_SRC_FILES := va_tpi.c

LOCAL_CFLAGS += -DANDROID

LOCAL_C_INCLUDES += \
	$(TARGET_OUT_HEADERS)/libva \
	$(LOCAL_PATH)/..

LOCAL_COPY_HEADERS_TO := libva/va

LOCAL_COPY_HEADERS := \
	va_tpi.h \
	va_backend_tpi.h

LOCAL_SHARED_LIBRARIES := libva

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libva-tpi

include $(BUILD_SHARED_LIBRARY)

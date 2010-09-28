# Recursive call sub-folder Android.mk
#
# include $(call all-subdir-makefiles)
LOCAL_PATH := $(my-dir)

include $(LOCAL_PATH)/va/Android.mk


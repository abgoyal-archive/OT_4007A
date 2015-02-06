LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	brfpatch.c

LOCAL_MODULE_TAGS := eng
LOCAL_MODULE := brfpatch

include $(BUILD_HOST_EXECUTABLE)

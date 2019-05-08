# Test ipu bi-cube, bilinear and output to fg1.

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES := lcdc_image_enhancement.c
LOCAL_SHARED_LIBRARIES := \
	libdmmu \
	libjzipu

LOCAL_MODULE := lcdc_image_enhancement

include $(BUILD_EXECUTABLE)

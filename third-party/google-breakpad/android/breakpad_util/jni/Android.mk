LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := breakpad_util_jni
LOCAL_SRC_FILES := breakpad_util.cpp
LOCAL_STATIC_LIBRARIES += breakpad_client
include $(BUILD_SHARED_LIBRARY)

# If NDK_MODULE_PATH is defined, import the module, otherwise do a direct
# includes. This allows us to build in all scenarios easily.
ifneq ($(NDK_MODULE_PATH),)
  $(call import-module,google_breakpad)
else
  include $(LOCAL_PATH)/../../google_breakpad/Android.mk
endif

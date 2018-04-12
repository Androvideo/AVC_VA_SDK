LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_32_BIT_ONLY := true

LOCAL_SRC_FILES := \
    va_tf_mobilenet_ssd.cpp \

#LOCAL_CFLAGS := -Wall -Wextra -std=c++11 -Wno-unused-variable -Wno-unused-parameter  -Wno-ignored-qualifiers
LOCAL_CFLAGS := -std=c++11 -Wno-ignored-qualifiers
LOCAL_CFLAGS += $(COMMON_FLAG)

LOCAL_CPPFLAGS := -DEBUG

LOCAL_C_INCLUDES := $(LOCAL_C_INCLUDES_COMMON)

LOCAL_CFLAGS += -D_USING_LIBCXX
LOCAL_CPPFLAGS += -nostdinc++
LOCAL_LDFLAGS += -nodefaultlibs
LOCAL_LDLIBS += -lm -lc -llog -ldl -lgcc
LOCAL_SHARED_LIBRARIES := libtensorflow
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_MODULE:= libva_tf_mobilenet_ssd

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libtensorflow
LOCAL_SRC_FILES := ../libs/libtensorflow.so
include $(PREBUILT_SHARED_LIBRARY)

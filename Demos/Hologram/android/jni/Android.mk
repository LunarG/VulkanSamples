PROJECT_TOP := ../../..
LOADER_TOP := $(PROJECT_TOP)/../LoaderAndTools
GLSLANG_TOP := $(PROJECT_TOP)/../glslang

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := Hologram

LOCAL_SRC_FILES := \
    ../../Animation.cpp \
    ../../HelpersDispatchTable.cpp \
    ../../Hologram.cpp \
    ../../Main.cpp \
    ../../Meshes.cpp \
    ../../Path.cpp \
    ../../Shell.cpp \
    ../../ShellAndroid.cpp

LOCAL_CPPFLAGS := \
	-DVK_USE_PLATFORM_ANDROID_KHR \
	-DVK_NO_PROTOTYPES

LOCAL_C_INCLUDES := \
	$(PROJECT_TOP)/ext \
	$(LOADER_TOP)/include

LOCAL_CPP_FEATURES := exceptions

LOCAL_LDLIBS := -landroid -llog -ldl
LOCAL_STATIC_LIBRARIES := android_native_app_glue

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)

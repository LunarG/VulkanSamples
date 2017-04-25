cmake_minimum_required(VERSION 3.4.1)

# build native_app_glue as a static lib
include_directories(${ANDROID_NDK}/sources/android/native_app_glue)
add_library(native_activity_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

# build the test vulkan app
set(WRAPPER_DIR ../../../common)

add_library(vktuts SHARED
    src/main/jni/TutorialValLayer.cpp
    src/main/jni/main.cpp
    ${WRAPPER_DIR}/vulkan_wrapper.cpp)

include_directories(${WRAPPER_DIR} ${SRC_DIR}/include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Werror \
    -Wno-unused-variable \
    -DVK_USE_PLATFORM_ANDROID_KHR \
    -DUSE_DEBUG_EXTENTIONS")

target_link_libraries(vktuts PRIVATE native_activity_glue log android)
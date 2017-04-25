PP_ABI := armeabi-v7a arm64-v8a x86 x86_64 mips mips64
APP_PLATFORM := android-24
# Shaderc builds the libs into a directory with APP_STL being part of it
# CMakeLists.ext would need to be updated if stl type is changed
APP_STL := gnustl_static
NDK_TOOLCHAIN_VERSION := clang


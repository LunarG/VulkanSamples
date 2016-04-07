# Vulkan Samples Kit
This repository is an Android port of [LunarG sample kit](https://github.com/LunarG/VulkanSamples).

## Prerequisites
- Android Studio 2.0
- ANdroid SDK N-preview or later
- NDK r12Beta or later

## Sample import
To import the samples, follow steps below:

Step 1: Build shaderc in NDK
In a command-prompt navigate to “${ndk_root}/sources/third_party/shaderc”
Run the following command

../../../ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=Android.mk APP_STL:=c++_static APP_ABI=all

APP_STL can be one of gnustl_static, gnustl_shared, c++_static, c++_shared.
Here, it’s going to use statically linked version of libC++ as samples are using it.

Step 2: Build Android Studio project file
In a command-prompt navigate to “LunarGSamples"
Run the following command
%  cmake -DANDROID=ON -DANDROID_ABI=[armeabi-v7a|arm64-v8a|x86|x86_64|all(default)]

Step 3: Import the samples in Android Studio by choosing “Import project (Eclipse, ADT, Gradle)” and choose:
VulkanSamples/API-Samples/android/build.gradle

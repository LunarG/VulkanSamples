# Vulkan Samples Kit
This repository is an Android port of [LunarG sample kit](https://github.com/LunarG/VulkanSamples).

## Prerequisites
- Android Studio 2.0
- ANdroid SDK N-preview or later
- NDK r12Beta or later

## Sample import
To import the samples, follow steps below:

Step 1: (Optional)Build shaderc in NDK. This step is optional for API_samples as Android Studio project automatically execute it. 
In a command-prompt navigate to “${ndk_root}/sources/third_party/shaderc”
Run the following command

../../../ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=Android.mk APP_STL:=c++_static APP_ABI=[armeabi-v7a|arm64-v8a|x86|x86_64|all] libshaderc_combined -j16

APP_STL can be one of gnustl_static, gnustl_shared, c++_static, c++_shared.
Here, it’s going to use statically linked version of libC++ as samples are using it.

Step 2: Sync external project and generate files.
In a command-prompt navigate to “LunarGSamples"
Run the following commands
./update_external_sources.sh -s -g

Step 3: (Optional) Build Android Studio project file. This step is optional as the repository includes a pregenerated project files that includes All ABIs.
cmake -DANDROID=ON -DANDROID_ABI=[armeabi-v7a|arm64-v8a|x86|x86_64|all(default)]

Step 4: Import the samples in Android Studio by choosing “Import project (Eclipse, ADT, Gradle)” and choose:
LunarGSamples/API-Samples/android/build.gradle

# Copyright 2016 The Android Open Source Project
# Copyright (C) 2016 Valve Corporation

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#      http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

APP_ABI := armeabi-v7a arm64-v8a x86 x86_64 mips mips64
APP_PLATFORM := android-24
APP_STL := gnustl_static
APP_CPPFLAGS += -std=c++11 -DVK_PROTOTYPES -Wall -Werror -Wno-unused-variable -Wno-unused-function -Wno-unused-const-variable -DUSE_DEBUG_EXTENTIONS -DVK_USE_PLATFORM_ANDROID_KHR
NDK_TOOLCHAIN_VERSION := clang


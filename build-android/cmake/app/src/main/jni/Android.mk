#Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)
#$(info "start building...")

include $(CLEAR_VARS)

TUTORIAL_COMMON := $(LOCAL_PATH)/../../../../../common/vulkan_wrapper

LOCAL_MODULE    := vktuts
LOCAL_SRC_FILES := main.cpp \
                   TutorialValLayer.cpp \
                   $(TUTORIAL_COMMON)/vulkan_wrapper.cpp
LOCAL_C_INCLUDES += $(NDK_ROOT)/sources/third_party/vulkan/src/include \
                    $(TUTORIAL_COMMON)

LOCAL_LDLIBS    := -llog -landroid
LOCAL_STATIC_LIBRARIES := android_native_app_glue

include $(BUILD_SHARED_LIBRARY)
$(call import-module,android/native_app_glue)

# $(info "done building")

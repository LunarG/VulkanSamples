#
# Copyright (C) 2016 The Android Open Source Project
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

cmake_minimum_required(VERSION 3.4.1)
# Build Util Lib
set(UTILS_NAME  vsamputils)
get_filename_component(PROJECT_SOURCE_DIR
                       "${CMAKE_SOURCE_DIR}/../../.."
                       ABSOLUTE)
get_filename_component(GLMINC_PREFIX
                       "${CMAKE_SOURCE_DIR}/../utils"
                       ABSOLUTE)
add_subdirectory(${CMAKE_SOURCE_DIR}/../utils ${CMAKE_BINARY_DIR}/utils)

# Build application's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11  -Werror -D VK_USE_PLATFORM_ANDROID_KHR")

# Force export ANativeActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

add_library(vulkan_sample SHARED
            ${CMAKE_SOURCE_DIR}/../${SAMPLE_NAME}/${SAMPLE_NAME}.cpp
            ${CMAKE_SOURCE_DIR}/vulkan_wrapper/vulkan_wrapper.cpp)

target_include_directories(vulkan_sample PRIVATE
                           ${CMAKE_SOURCE_DIR}/../data
                           ${GLMINC_PREFIX}
                           ${CMAKE_SOURCE_DIR}/ShaderHeaders
                           ${CMAKE_SOURCE_DIR}/../android/vulkan_wrapper)

target_link_libraries(vulkan_sample
                      log
                      ${UTILS_NAME})

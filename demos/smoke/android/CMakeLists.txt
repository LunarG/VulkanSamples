#
# Copyright (C) 2017 The Android Open Source Project
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

project(Smoke)
cmake_minimum_required(VERSION 3.4.1)

get_filename_component(demosDir "${CMAKE_SOURCE_DIR}/../.." ABSOLUTE)
set(smokeDir "${demosDir}/smoke")
get_filename_component(glmDir "${demosDir}/../libs" ABSOLUTE)
get_filename_component(vulkanDir "${demosDir}/../include" ABSOLUTE)

# build native_app_glue as a static lib
add_library(native_activity_glue STATIC
            ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

# Build application's shared lib
set(CMAKE_CXX_FLAGS
            "${CMAKE_CXX_FLAGS} -std=c++11  -fexceptions -Wall \
            -Wextra -Wno-unused-parameter \
            -DVK_NO_PROTOTYPES -DVK_USE_PLATFORM_ANDROID_KHR \
            -DGLM_FORCE_RADIANS")

# Force export ANativeActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

add_library(Smoke SHARED
            ${smokeDir}/Game.cpp
            ${smokeDir}/Meshes.cpp
            ${smokeDir}/Simulation.cpp
            ${smokeDir}/HelpersDispatchTable.cpp
            ${smokeDir}/Shell.cpp
            ${smokeDir}/ShellAndroid.cpp
            ${smokeDir}/Smoke.cpp
            ${smokeDir}/Main.cpp)

target_include_directories(Smoke PRIVATE
            ${ANDROID_NDK}/sources/android/native_app_glue
            ${vulkanDir}
            ${glmDir}
            ${CMAKE_SOURCE_DIR}/src/main/jni)

target_link_libraries(Smoke
            android
            log
            native_activity_glue)
